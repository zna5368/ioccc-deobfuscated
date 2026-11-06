#include<math.h>
#include<stdio.h>

/*
 * OMLET - a tiny character level recurrent neural network.
 *
 * This is the default "prog" network: a single layer vanilla RNN
 *
 *     h = tanh(Wxh * x + Whh * h' + Bh)
 *     y = Why * h + By
 *
 * The network is described as a list of operations on vectors.  All vectors
 * of one time step live in a "frame" of 2 * frame_size floats: the first half
 * holds the activations and the second half holds their gradients (or, for
 * the output vector, the softmax probabilities).
 */

/* hyperparameters (normally given with -D on the compiler command line) */
#ifndef RS
#define RS .15
#endif
#ifndef TR
#define TR .95
#endif
#ifndef LR
#define LR .002
#endif
#ifndef LD
#define LD .97
#endif
#ifndef LE
#define LE 10
#endif
#ifndef CL
#define CL 5
#endif
#ifndef B1
#define B1 .9
#endif
#ifndef B2
#define B2 .999
#endif
#ifndef EP
#define EP 1e-8
#endif
#ifndef WD
#define WD 8e-5
#endif
#ifndef DI
#define DI 100
#endif
#ifndef SL
#define SL 200
#endif
#ifndef N
#define N 50
#endif
#ifndef TP
#define TP 1
#endif
#ifndef PF
#define PF "cp%02d_%.3f"
#endif

#define INIT_RANGE         RS   /* weights start in [-RS, RS] */
#define TRAIN_FRACTION     TR   /* part of the batches used for training */
#define LEARNING_RATE      LR
#define DECAY_RATE         LD   /* learning rate decay per epoch */
#define DECAY_EPOCH        LE   /* first epoch with learning rate decay */
#define GRAD_CLIP          CL
#define ADAM_BETA1         B1
#define ADAM_BETA2         B2
#define ADAM_EPSILON       EP
#define WEIGHT_DECAY       WD
#define REPORT_INTERVAL    DI   /* batches between progress reports */
#define SAMPLE_LENGTH      SL   /* characters printed in a sample */
#define SEQ_LEN            N    /* characters per batch (unroll depth) */
#define TEMPERATURE        TP
#define CHECKPOINT_FORMAT  PF

#define HIDDEN_SIZE 128
#define MAX_OPS     99

/* a vector inside a frame (or a block of parameters) */
typedef struct {
    int offset, size;
} Vector;

enum {
    OP_LINEAR,      /* out = W * in + B                         */
    OP_TANH,        /* out = tanh(in)                           */
    OP_SIGMOID,     /* out = 1 / (1 + exp(-in))                 */
    OP_CARRY,       /* next frame's "in" = out                  */
    OP_ADD,         /* out = in + in2                           */
    OP_MULTIPLY,    /* out = in * in2                           */
    OP_OFFSET_GAIN, /* out = in * param.size + param.offset     */
    OP_GAIN         /* out = in * W (element-wise)              */
};

/*
 * One operation.  "param" is the block of learnable parameters for linear
 * and gain operations, the second input for add and multiply, and the two
 * constants for offset-gain.
 */
typedef struct {
    int type;
    Vector out, in, param;
} Op;

int frame_size;          /* floats of activations in one time step */
int param_count;         /* number of learnable parameters */
int train_batches;       /* batches in the training part of the text */
int batch_number;        /* current batch within the epoch */
int last_char;           /* last generated (or seen) character */

Op ops[MAX_OPS], *ops_end = ops;

float *frame;            /* frame of the current time step */
float *next_frame;       /* frame that receives the carried state */
float *saved_state;      /* state carried from one batch to the next */
float *params;           /* parameters, gradients, Adam variance, Adam mean */
float beta1_power = 1, beta2_power = 1;
float loss;

Vector net_input, net_output;

double drand48();

unsigned char char_to_index[256];
unsigned char batch[sizeof char_to_index + SEQ_LEN];

/* checkpoint header */
struct {
    float learning_rate;
    short alphabet_size, epoch;
    unsigned char index_to_char[sizeof char_to_index];
} header = { LEARNING_RATE };

/* reserve a new vector of the given size in the frame */
Vector new_vector(int size)
{
    Vector v = { frame_size, size };
    frame_size += size;
    return v;
}

Vector add_op(int type, Vector in, Vector param)
{
    Vector out = new_vector(in.size);
    *ops_end++ = (Op){ type, out, in, param };
    return out;
}

Vector add(Vector a, Vector b)        { return add_op(OP_ADD, a, b); }
Vector multiply(Vector a, Vector b)   { return add_op(OP_MULTIPLY, a, b); }
Vector sigmoid_layer(Vector in)       { return add_op(OP_SIGMOID, in, (Vector){ 0, 0 }); }
Vector tanh_layer(Vector in)          { return add_op(OP_TANH, in, (Vector){ 0, 0 }); }

/* constants: the result is in * offset + gain */
Vector offset_gain(int offset, int gain, Vector in)
{
    return add_op(OP_OFFSET_GAIN, in, (Vector){ gain, offset });
}

/* copy "value" into "previous" of the next time step */
Vector carry(Vector previous, Vector value)
{
    *ops_end++ = (Op){ OP_CARRY, value, previous };
    return value;
}

/* fully connected layer: size biases plus size * in.size weights */
Vector linear(int size, Vector in)
{
    Vector out = new_vector(size);
    Vector weights = { param_count, size + in.size * size };
    param_count += weights.size;
    *ops_end++ = (Op){ OP_LINEAR, out, in, weights };
    return out;
}

/* learnable element-wise gain */
Vector gain(Vector in)
{
    Vector out = new_vector(in.size);
    Vector weights = { param_count, in.size };
    param_count += in.size;
    *ops_end++ = (Op){ OP_GAIN, out, in, weights };
    return out;
}

void zero_floats(float *p, int count)
{
    while (count--)
        *p++ = 0;
}

/* run the network for one time step, leaving the softmax in the gradients */
void forward(int input_index)
{
    Op *op;
    int k, n;
    float max, sum;
    float *input = frame + net_input.offset;

    zero_floats(input, net_input.size);
    input[input_index]++;

    for (op = ops; op < ops_end; ++op) {
        float *out = frame + op->out.offset;
        float *in = frame + op->in.offset;
        float *in2 = frame + op->param.offset;
        float *weight = params + op->param.offset;
        float *carried = next_frame + op->in.offset;

        switch (op->type) {
        case OP_LINEAR:
            for (k = op->out.size; k-- > 0;) {
                out[k] = *weight++;
                for (n = op->in.size; n-- > 0;)
                    out[k] += *weight++ * in[n];
            }
            break;
        case OP_TANH:
            for (k = op->out.size; k-- > 0;)
                out[k] = tanh(in[k]);
            break;
        case OP_SIGMOID:
            for (k = op->out.size; k-- > 0;)
                out[k] = 1 / (exp(-in[k]) + 1);
            break;
        case OP_CARRY:
            for (k = op->out.size; k-- > 0;)
                carried[k] = out[k];
            break;
        case OP_ADD:
            for (k = op->out.size; k-- > 0;)
                out[k] = in[k] + in2[k];
            break;
        case OP_MULTIPLY:
            for (k = op->out.size; k-- > 0;)
                out[k] = in[k] * in2[k];
            break;
        case OP_OFFSET_GAIN:
            for (k = op->out.size; k-- > 0;)
                out[k] = in[k] * op->param.size + op->param.offset;
            break;
        case OP_GAIN:
            for (k = op->out.size; k-- > 0;)
                out[k] = in[k] * *weight++;
            break;
        }
    }

    zero_floats(frame + frame_size, frame_size);

    /* softmax of the output vector, stored where its gradient goes */
    float *logits = frame + net_output.offset;
    float *probs = logits + frame_size;

    max = logits[0];
    for (k = net_output.size; k-- > 0;)
        max = fmax(max, logits[k]);
    sum = 0;
    for (k = net_output.size; k-- > 0;) {
        probs[k] = exp((logits[k] - max) / TEMPERATURE);
        sum += probs[k];
    }
    for (k = net_output.size; k-- > 0;)
        probs[k] /= sum;
}

/* run the network and pick a random next character from its prediction */
int sample_next(int input_index)
{
    int k;
    float *probs = frame + net_output.offset + frame_size;
    float r;

    forward(input_index);
    r = drand48();
    for (k = net_output.size; k-- > 0;)
        if ((r -= probs[k]) < 0)
            break;
    return last_char = k;
}

/* propagate the gradients of one time step backwards through the network */
void backward(void)
{
    Op *op;
    int k, n;

    for (op = ops_end; op-- > ops;) {
        float *out = frame + op->out.offset;
        float *dout = out + frame_size;
        float *in = frame + op->in.offset;
        float *din = in + frame_size;
        float *in2 = frame + op->param.offset;
        float *din2 = in2 + frame_size;
        float *weight = params + op->param.offset;
        float *dweight = weight + param_count;
        float *dcarried = next_frame + op->in.offset + frame_size;

        switch (op->type) {
        case OP_LINEAR:
            for (k = op->out.size; k-- > 0;) {
                *dweight++ += dout[k];
                weight++;
                for (n = op->in.size; n-- > 0;) {
                    *dweight++ += dout[k] * in[n];
                    din[n] += dout[k] * *weight++;
                }
            }
            break;
        case OP_TANH:
            for (k = op->out.size; k-- > 0;)
                din[k] += dout[k] * (1 - out[k] * out[k]);
            break;
        case OP_SIGMOID:
            for (k = op->out.size; k-- > 0;)
                din[k] += dout[k] * (1 - out[k]) * out[k];
            break;
        case OP_CARRY:
            for (k = op->out.size; k-- > 0;)
                dout[k] += dcarried[k];
            break;
        case OP_ADD:
            for (k = op->out.size; k-- > 0;) {
                din[k] += dout[k];
                din2[k] += dout[k];
            }
            break;
        case OP_MULTIPLY:
            for (k = op->out.size; k-- > 0;) {
                din[k] += in2[k] * dout[k];
                din2[k] += in[k] * dout[k];
            }
            break;
        case OP_OFFSET_GAIN:
            for (k = op->out.size; k-- > 0;)
                din[k] += op->param.size * dout[k];
            break;
        case OP_GAIN:
            /* note: the weight gradient uses the output, not the input */
            for (k = op->out.size; k-- > 0;) {
                din[k] += dout[k] * *weight++;
                *dweight++ += dout[k] * out[k];
            }
            break;
        }
    }
}

/*
 * Backpropagation through time.  Each recursion level is one time step and
 * keeps the frame of the following time step on the stack.
 */
void train_sequence(unsigned char *text)
{
    float following[2 * frame_size];
    float *current = frame;
    float *saved_next = next_frame;
    float *probs;
    int target, k;

    next_frame = following;
    forward(char_to_index[*text++]);

    /* cross entropy loss; the softmax gradient is probability - 1 */
    target = char_to_index[*text];
    probs = frame + net_output.offset + frame_size;
    loss -= log(probs[target]) / SEQ_LEN;
    probs[target] -= 1;

    frame = following;
    if (text[1]) {
        train_sequence(text);
    } else {
        /* end of batch: keep the state for the next batch */
        for (k = frame_size; k-- > 0;) {
            saved_state[k] = frame[k];
            frame[frame_size + k] = 0;
        }
    }
    frame = current;

    if (batch_number < train_batches)
        backward();

    next_frame = saved_next;
}

/* Adam optimizer step with gradient clipping and weight decay */
void update_params(void)
{
    float *p, *grad, *variance, *mean;
    float g;

    for (p = params + param_count; p-- > params;) {
        grad = p + param_count;
        variance = p + 2 * param_count;
        mean = p + 3 * param_count;

        g = fmax(-GRAD_CLIP, fmin(GRAD_CLIP, *grad));
        beta1_power *= ADAM_BETA1;
        *mean = g + (*mean - g) * ADAM_BETA1;
        beta2_power *= ADAM_BETA2;
        g *= g;
        *variance = g + (*variance - g) * ADAM_BETA2;
        *p += (*mean / sqrt(*variance / (1 - beta2_power) + ADAM_EPSILON)
               / (beta1_power - 1) + WEIGHT_DECAY * *p) * header.learning_rate;
        *grad = 0;
    }
}

/* the user module: one RNN layer */
Vector block(Vector x)
{
    Vector previous_hidden = new_vector(HIDDEN_SIZE);
    Vector from_input = linear(HIDDEN_SIZE, x);
    Vector hidden = carry(previous_hidden,
                          tanh_layer(add(from_input,
                                         linear(HIDDEN_SIZE, previous_hidden))));
    return hidden;
}

int main(int argc, char **argv)
{
    FILE *text = NULL, *checkpoint = stdin;
    int training = argc - 1;
    int ch, k, char_count = 0, valid_batches;
    unsigned char seen[256] = { 0 };

    if (training) {
        checkpoint = fopen(argv[2], "r");
        text = fopen(argv[1], "r");

        /* build the alphabet (stops at the end of file or a NUL byte) */
        while ((ch = getc(text)) > 0) {
            ++char_count;
            if (!seen[ch]) {
                seen[ch]++;
                header.index_to_char[header.alphabet_size] = ch;
                char_to_index[ch] = header.alphabet_size++;
            }
        }
    }
    if (checkpoint)
        fread(&header, sizeof header, 1, checkpoint);

    net_input = new_vector(header.alphabet_size);
    net_output = linear(header.alphabet_size, block(net_input));

    float state[frame_size + frame_size];
    float all_params[4 * param_count];
    float first_frame[frame_size * 2];

    frame = next_frame = saved_state = state;
    params = all_params;
    zero_floats(params, 4 * param_count);
    for (k = param_count; k-- > 0;)
        params[k] = 2 * drand48() * INIT_RANGE - INIT_RANGE;
    if (checkpoint)
        fread(params, 1, sizeof all_params, checkpoint);
    zero_floats(frame, frame_size);

    char_count /= SEQ_LEN;
    train_batches = char_count * TRAIN_FRACTION;
    valid_batches = char_count - train_batches;
    batch[SEQ_LEN + 1] = 0;

    for (; training; fclose(checkpoint)) {
        rewind(text);
        while (fread(batch, SEQ_LEN + 1, 1, text)) {
            frame = first_frame;
            for (k = frame_size; k-- > 0;)
                frame[k] = saved_state[k];
            train_sequence(batch);

            /* the last character also starts the next batch */
            ungetc(last_char = batch[SEQ_LEN], text);

            if (batch_number++ < train_batches) {
                update_params();
                if (batch_number % REPORT_INTERVAL == 0) {
                    frame = saved_state;
                    for (k = 0; k < SAMPLE_LENGTH; k++)
                        putchar(header.index_to_char[sample_next(last_char)]);
                }
            }
            if (batch_number % REPORT_INTERVAL == 0) {
                int done = batch_number, total = train_batches;
                int validating = done > total;

                if (validating) {
                    done -= total;
                    total = valid_batches;
                }
                last_char = validating;
                printf(validating ? "%c%d:%d%% %f\n" : "\n%c%d:%d%% %f\n",
                       "TV"[validating], header.epoch, 100 * done / total,
                       loss / done);
            }
            /* restart the loss sum for the validation part */
            loss *= batch_number != train_batches;
        }

        char filename[999];

        if (DECAY_EPOCH <= ++header.epoch)
            header.learning_rate *= DECAY_RATE;
        sprintf(filename, CHECKPOINT_FORMAT, header.epoch, loss / valid_batches);
        checkpoint = fopen(filename, "w");
        fwrite(&header, sizeof header, 1, checkpoint);
        batch_number = 0;
        loss = 0;
        fwrite(all_params, param_count, sizeof *params, checkpoint);
    }

    /* inference: print characters forever */
    while (putchar(header.index_to_char[sample_next(last_char)])) {
    }
}
