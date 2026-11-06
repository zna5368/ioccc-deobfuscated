#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>
#include<sys/mman.h>

/*
 * ChatIOCCC - a LLaMA 2 (7B, chat) inference engine using 8-bit
 * group-quantized weights.
 */

#ifndef M
#define M "model"
#endif
#define MODEL_FILE M

/* system prompt, chosen at compile time with -DS="..." */
#ifndef S
#define S ""
#endif

/* model dimensions of llama2-7b */
enum {
    N_LAYERS   = 32,
    N_HEADS    = 32,
    GROUP_SIZE = 64,             /* values sharing one quantization scale */
    HEAD_DIM   = 128,
    DIM        = 4096,           /* N_HEADS * HEAD_DIM, also the max context */
    HIDDEN_DIM = 11008,          /* feed forward hidden size (86 * 128) */
    VOCAB_SIZE = 32000,
    MAX_POS    = DIM             /* context window length */
};

/* special tokens */
enum { TOKEN_UNK = 0, TOKEN_BOS = 1, TOKEN_EOS = 2 };

/* an 8-bit quantized matrix: int8 values plus one float scale per group */
typedef struct {
    int8_t *values;
    float *scales;
} QTensor;

int open();                      /* declared by hand: <fcntl.h> is not used */

/* token ids of the greeting printed at start up
 * ("Welcome to ChatIOCCC! Ask me anything.") */
static const int banner_tokens[] = {
    28862, 2763, 304, 16579, 29873, 5971, 4174, 29907, 29991,
    26579, 592, 3099, 29889, 0
};

/* tokenizer */
char *vocab[VOCAB_SIZE];
char prompt_text[HIDDEN_DIM];
char user_input[DIM];
int prompt_tokens[DIM];
int n_prompt_tokens;
int prompt_pos;

/* weights (all inside the memory mapped model file) */
QTensor token_embedding;
QTensor classifier;
QTensor attn_weights[4][N_LAYERS];   /* wq, wk, wv, wo */
QTensor ffn_weights[3][N_LAYERS];    /* w1 (gate), w3 (up), w2 (down) */
float *norm_weights;                 /* attention norms, ffn norms, final */

/* key / value cache: [layer][position][DIM] */
float *key_cache;
float *value_cache;

/* activations */
float x[DIM];                        /* residual stream */
float xb[DIM];                       /* normalized input / attention output */
float attn_out[DIM];
float q[DIM];
float hb[HIDDEN_DIM];
float hb2[HIDDEN_DIM];
float logits[VOCAB_SIZE];
float att[N_HEADS][MAX_POS];         /* attention weights of each head */

/* quantized copy of a matrix multiply input */
int8_t xq[HIDDEN_DIM];
float xq_scales[HIDDEN_DIM / GROUP_SIZE];

/* reads n_rows x DIM quantized values followed by their scales */
static QTensor next_tensor(float **model, int n_rows)
{
    QTensor tensor;
    tensor.values = (int8_t *)*model;
    tensor.scales = *model + n_rows * DIM / 4;
    *model += n_rows * DIM / 4 + n_rows * DIM / GROUP_SIZE;
    return tensor;
}

/* out = x / rms(x) * weight */
static void rmsnorm(float *out, const float *weight)
{
    float ss = 1e-5;
    int i;

    for (i = 0; i < DIM; i++)
        ss += x[i] * x[i] / DIM;
    for (i = 0; i < DIM; i++)
        out[i] = x[i] / sqrt(ss) * weight[i];
}

/* converts n_groups * GROUP_SIZE floats into int8 values with a scale */
static void quantize(const float *in, int n_groups)
{
    int g, i;

    for (g = 0; g < n_groups; g++) {
        float scale = 0;
        for (i = 0; i < GROUP_SIZE; i++)
            scale = fmax(fabs(in[i]) / 127, scale);
        xq_scales[g] = scale;
        for (i = 0; i < GROUP_SIZE; i++)
            xq[i + g * GROUP_SIZE] = *in++ / scale;
    }
}

/* out[n_rows] = W * xq where each row of W has n_groups groups */
static void matmul(float *out, const QTensor *w, int n_rows, int n_groups)
{
#pragma omp parallel for
    for (int row = 0; row < n_rows; row++) {
        for (int g = 0; g < n_groups; g++) {
            int sum = 0, group = row * n_groups + g;
            for (int i = 0; i < GROUP_SIZE; i++)
                sum += w->values[group * GROUP_SIZE + i] * xq[g * GROUP_SIZE + i];
            float partial = sum * w->scales[group] * xq_scales[g];
            out[row] = g ? partial + out[row] : partial;
        }
    }
}

/* rotary position embedding of one DIM vector */
static void rope(float *vec, int pos)
{
    for (int i = 0; i < DIM; i += 2) {
        /* 10000^(i/128) is approximately exp(i/14) */
        float angle = pos / exp(i % HEAD_DIM / 14.);
        float sn = sin(angle);
        float cs = cos(angle);
        float v0 = vec[i], v1 = vec[i + 1];
        vec[i + 1] = sn * v0 + v1 * cs;
        vec[i] = cs * v0 - v1 * sn;
    }
}

/* runs one token through the transformer, leaving scores in logits */
static void forward(int token, int pos)
{
    int i;

    for (i = 0; i < DIM; i++) {
        int index = token * DIM + i;
        x[i] = token_embedding.values[index] * token_embedding.scales[index / GROUP_SIZE];
    }

    for (int layer = 0; layer < N_LAYERS; layer++) {
        int layer_offset = layer * DIM * DIM;
        float *key = key_cache + layer_offset + pos * DIM;
        float *value = value_cache + layer_offset + pos * DIM;

        /* query, key and value of the normalized input */
        rmsnorm(xb, norm_weights + layer * DIM);
        quantize(xb, DIM / GROUP_SIZE);
        matmul(q, &attn_weights[0][layer], DIM, DIM / GROUP_SIZE);
        matmul(key, &attn_weights[1][layer], DIM, DIM / GROUP_SIZE);
        matmul(value, &attn_weights[2][layer], DIM, DIM / GROUP_SIZE);

        rope(key, pos);
        rope(q, pos);

        /* multi-head attention over every position so far */
#pragma omp parallel for
        for (int h = 0; h < N_HEADS; h++) {
            float *score = att[h];
            float sum = 0;
            int head = h * HEAD_DIM;
            int t, k;

            for (k = 0; k < HEAD_DIM; k++)
                xb[head + k] = 0;
            for (t = 0; t <= pos; t++) {
                /* score[t] is not reset, it still holds the
                 * weight from the previous token */
                for (k = 0; k < HEAD_DIM; k++)
                    score[t] += q[head + k] *
                        key_cache[layer_offset + t * DIM + head + k] / 11;
                sum += score[t] = exp(score[t]);
            }
            for (t = 0; t <= pos; t++) {
                score[t] /= sum;
                for (k = 0; k < HEAD_DIM; k++)
                    xb[head + k] += score[t] *
                        value_cache[layer_offset + t * DIM + head + k];
            }
        }

        quantize(xb, DIM / GROUP_SIZE);
        matmul(attn_out, &attn_weights[3][layer], DIM, DIM / GROUP_SIZE);
        for (i = 0; i < DIM; i++)
            x[i] += attn_out[i];

        /* feed forward network: w2(silu(w1(x)) * w3(x)) */
        rmsnorm(xb, norm_weights + (N_LAYERS + layer) * DIM);
        quantize(xb, DIM / GROUP_SIZE);
        matmul(hb, &ffn_weights[0][layer], HIDDEN_DIM, DIM / GROUP_SIZE);
        matmul(hb2, &ffn_weights[1][layer], HIDDEN_DIM, DIM / GROUP_SIZE);
        for (i = 0; i < HIDDEN_DIM; i++)
            hb[i] *= hb2[i] / (exp(-hb[i]) + 1);
        quantize(hb, HIDDEN_DIM / GROUP_SIZE);
        matmul(xb, &ffn_weights[2][layer], DIM, HIDDEN_DIM / GROUP_SIZE);
        for (i = 0; i < DIM; i++)
            x[i] += xb[i];
    }

    rmsnorm(x, norm_weights + 2 * N_LAYERS * DIM);
    quantize(x, DIM / GROUP_SIZE);
    matmul(logits, &classifier, VOCAB_SIZE, DIM / GROUP_SIZE);
}

/* greedy sampling: the token with the highest score */
static int argmax(void)
{
    int best = 0;

    for (int i = 1; i < VOCAB_SIZE; i++)
        if (logits[i] > logits[best])
            best = i;
    return best;
}

/* greedy longest match tokenizer: appends the tokens of text */
static void tokenize(const char *text, int length)
{
    while (length > 0) {
        int len, token = 0;
        for (len = length; ; len--) {
            for (token = VOCAB_SIZE - 1; token > 0; token--)
                if (vocab[token][len] == 0 && memcmp(vocab[token], text, len) == 0)
                    break;
            if (token)
                break;
        }
        prompt_tokens[n_prompt_tokens++] = token;
        text += len;
        length -= len;
    }
}

/* reads one line from the user and turns it into prompt tokens */
static void read_prompt(int pos)
{
    int length;

    printf("\n\n> ");
    if (scanf("%[^\n]%*c", user_input) > 0) {
        prompt_tokens[0] = TOKEN_BOS;
        n_prompt_tokens = 1;
    } else
        exit(0);

    /* LLaMA 2 chat template, the system prompt only on the first turn */
    length = sprintf(prompt_text, "[INST] %s%s [/INST]",
        pos ? "" : "<<SYS>>\n" S "\n<</SYS>>\n\n", user_input);
    tokenize(prompt_text, length);
    prompt_pos = 0;
}

int main()
{
    float *model;
    int fd, i, layer, token, pos = 0;

    fd = open(MODEL_FILE, 0 /* O_RDONLY */);
    model = mmap(0, 8000000000, PROT_READ, MAP_PRIVATE, fd, 0);
    if (fd == -1)
        exit(puts(MODEL_FILE " not found"));
    key_cache = malloc(3000000000);
    value_cache = malloc(3000000000);

    /* vocabulary: each entry starts with its size in floats, then a string */
    for (i = 0; i < VOCAB_SIZE; i++) {
        vocab[i] = (char *)model + 4;
        model += (int)*model;
    }

    token_embedding = next_tensor(&model, VOCAB_SIZE);
    classifier = next_tensor(&model, VOCAB_SIZE);
    for (i = 0; i < 4; i++)
        for (layer = 0; layer < N_LAYERS; layer++)
            attn_weights[i][layer] = next_tensor(&model, DIM);
    for (i = 0; i < 3; i++)
        for (layer = 0; layer < N_LAYERS; layer++)
            ffn_weights[i][layer] = next_tensor(&model, HIDDEN_DIM);
    norm_weights = model;

    for (i = 0; banner_tokens[i]; i++)
        printf("%s", vocab[banner_tokens[i]]);

    token = TOKEN_UNK;
    for (;;) {
        /* ask for input when the model has finished its answer */
        if (token <= TOKEN_EOS && prompt_pos >= n_prompt_tokens)
            read_prompt(pos);

        if (prompt_pos < n_prompt_tokens)
            token = prompt_tokens[prompt_pos++];
        else
            fflush(0);

        forward(token, pos++);
        token = argmax();

        if (pos >= MAX_POS) {
            /* context window is full: forget everything */
            token = pos = 0;
        } else if (prompt_pos >= n_prompt_tokens)
            printf("%s", vocab[token]);
    }
}
