#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

/*
 * A multilayer perceptron with 65536 inputs (one per byte pair / bigram),
 * 6 hidden neurons and 1 output neuron.
 *
 * The network is stored as float network[7][65536]:
 *   rows 0..5  weights from every input to hidden neuron 0..5
 *   row  6     "scratch" row: the output neuron's weights live in the first
 *              six slots, and the remaining (otherwise unused) slots hold
 *              temporary values used during evaluation and training.
 * The whole array, scratch values included, is what gets read from stdin and
 * written to stdout.
 */
#define INPUTS 65536
#define HIDDEN 6
#define LEARNING_RATE .3

/* slots in the scratch row network[HIDDEN] */
enum {
    OUT_WEIGHTS = 0,  /* [0..5]    weight of each hidden neuron -> output */
    ERROR = 13,       /*           target - output                      */
    OUT_DELTA = 14,   /*           output error gradient                */
    HIDDEN_DELTA = 34,/*           gradient of the current hidden neuron */
    HIDDEN_DERIV = 50,/* [50..55]  derivative h*(1-h) of hidden neurons */
    OUTPUT = 81,      /*           output neuron activation             */
    OUT_DERIV = 82,   /*           derivative o*(1-o) of the output     */
    TOTAL_ERROR = 97, /*           sum of squared errors of an epoch    */
    /*
     * Training reads the hidden derivatives from here instead of from
     * HIDDEN_DERIV ((1<<7)-14 rather than (1<<6)-14). Nothing ever writes
     * these slots, so they just hold whatever the network file (or the random
     * initialisation) put there.
     */
    HIDDEN_DERIV_USED = 114
};

/*
 * A sample is an array of 2*INPUTS floats: [0..INPUTS) holds the normalised
 * bigram frequencies, and [INPUTS..INPUTS+HIDDEN) receives the hidden neuron
 * activations when the sample is fed through the network.
 */

/* sum of a[i]*b[i], accumulated from the last element down to the first */
float dot_product(float *a, int n, float *b)
{
    float sum = 0;
    int i;

    for (i = n - 1; i >= 0; --i)
        sum += b[i] * a[i];
    return sum;
}

/* build a normalised bigram histogram of everything left on stdin */
float *read_sample(void)
{
    float *sample = calloc(sizeof(float), 2 * INPUTS);
    int prev = getc(stdin), cur, pairs = 0, i;

    while ((cur = getc(stdin)) != EOF) {
        ++sample[(prev << 8) + cur];
        ++pairs;
        prev = cur;
    }
    if (pairs)
        for (i = 0; i < INPUTS; ++i)
            sample[i] /= pairs;
    return sample;
}

/* feed a sample forward through the network and return the output (0..1) */
float evaluate(float *sample, float network[][INPUTS])
{
    float *scratch = network[HIDDEN];
    float *hidden = sample + INPUTS;
    int j;

    for (j = 0; j < HIDDEN; ++j) {
        hidden[j] = 1 / (1 + expf(-dot_product(network[j], INPUTS, sample)));
        scratch[HIDDEN_DERIV + j] = hidden[j] * (1. - hidden[j]);
    }
    scratch[OUTPUT] = 1 / (1 + expf(-dot_product(hidden, HIDDEN, scratch + OUT_WEIGHTS)));
    scratch[OUT_DERIV] = scratch[OUTPUT] * (1. - scratch[OUTPUT]);
    return scratch[OUTPUT];
}

/* one step of on-line backpropagation; returns the squared error */
float train(float target, float network[][INPUTS], float *sample)
{
    float *scratch = network[HIDDEN];
    float *hidden = sample + INPUTS;
    int j, i;

    scratch[ERROR] = target - evaluate(sample, network);
    scratch[OUT_DELTA] = scratch[OUT_DERIV] * scratch[ERROR];
    for (j = 0; j < HIDDEN; ++j) {
        scratch[HIDDEN_DELTA] = scratch[HIDDEN_DERIV_USED + j] * scratch[OUT_DELTA]
                                * scratch[OUT_WEIGHTS + j];
        for (i = 0; i < INPUTS; ++i)
            network[j][i] += scratch[HIDDEN_DELTA] * LEARNING_RATE * sample[i];
        scratch[OUT_WEIGHTS + j] += LEARNING_RATE * scratch[OUT_DELTA] * hidden[j];
    }
    return powf(scratch[ERROR], 2);
}

/*
 * Load every non-hidden file in a directory (whose name must end in '/')
 * as a sample. Returns a NULL-terminated array of samples.
 */
float **load_corpus(char *dirname)
{
    DIR *dir = opendir(dirname);
    int count = 0;
    struct dirent *entry;
    float **samples = 0;

    while (dir ? (entry = readdir(dir)) : 0) {
        if (entry->d_name[0] != '.') {
            char *path = malloc(strlen(dirname) + strlen(entry->d_name) + 1);
            sprintf(path, "%s%s", dirname, entry->d_name);
            if (freopen(path, "r", stdin)) {
                samples = realloc(samples, sizeof(float *) * (count + 1));
                samples[count++] = read_sample();
            }
        }
    }
    samples = realloc(samples, (1 + count) * sizeof(float *));
    samples[count] = 0;
    return samples;
}

int main(int argc, char **argv)
{
    float network[HIDDEN + 1][INPUTS], ***corpora;
    int c, s, iter, ncorpora;

    /* random starting network, replaced by stdin if a network is given */
    srand(time(0));
    for (c = 0; c < HIDDEN + 1; ++c)
        for (s = 0; s < INPUTS; ++s)
            network[c][s] = rand() / (float)RAND_MAX - .5;
    fread(network, sizeof(network), 1, stdin);

    if (argv[1][0] == '-') {
        /* training: dlowe -<iterations> corpus1/ ... corpus0/ */
        ncorpora = argc - 2;
        corpora = malloc(sizeof(float **) * ncorpora);
        for (c = 0; c < ncorpora; ++c)
            corpora[c] = load_corpus(argv[2 + c]);
        for (iter = 0; iter < atoi(&argv[1][1]); ++iter) {
            network[HIDDEN][TOTAL_ERROR] = 0;
            for (c = 0; c < ncorpora; ++c)
                for (s = 0; corpora[c][s]; ++s)
                    network[HIDDEN][TOTAL_ERROR] +=
                        train(1. - (float)c / (ncorpora - 1), network, corpora[c][s]);
            fprintf(stderr, "%d: %f\n", iter, network[HIDDEN][TOTAL_ERROR]);
        }
        fwrite(network, sizeof(network), 1, stdout);
    } else {
        /* classifying: dlowe file ... */
        for (c = 1; c < argc; ++c)
            if (freopen(argv[c], "r", stdin))
                fprintf(stderr, "%s %f\n", argv[c], evaluate(read_sample(), network));
    }
    return 0;
}
