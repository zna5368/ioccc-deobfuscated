#include <stdlib.h>
#include <stdio.h>

/*
 * GLICBAWLS - Grey Level Image Compression By Adaptive Weighted Least Squares
 *
 * Reads a PGM/PPM image (P2, P3, P5, P6) and writes a compressed stream, or
 * reads a compressed stream (P13, P12, P10, P9) and writes the image back.
 * The compressed header is the image header with the magic number replaced
 * by 15 - number, so compressing and decompressing share the same code.
 *
 * Usage: glicbawls [-maxerror] [columns] < input > output
 *
 * All the floating point arithmetic below is done in exactly the same order
 * as in the original program so that compressed files stay compatible.
 */

double sqrt(double);

#define NEIGHBOURS 12              /* pixels used by the predictor          */
#define DIM 13                     /* NEIGHBOURS + the pixel itself         */
#define MATRIX (DIM * DIM)         /* one 13x13 correlation matrix          */
#define DEGREES 13                 /* Student t distribution shape          */
#define HALF 65536                 /* arithmetic coder: half of the range   */
#define BYTE_BASE 256              /* output byte is ready when >= this     */
#define PRED_DECAY .8              /* weight 0.8^distance for the predictor */
#define ERROR_DECAY .7             /* weight 0.7^distance for sigma         */

/* ---------------------------------------------------------------------- */
/* Input and output                                                        */
/* ---------------------------------------------------------------------- */

/*
 * Values written to stdout are first put into out_value.  The next call to
 * flush_output() prints it if it is >= out_base and then replaces it with the
 * return value of printf(), which is always below out_base again.
 *
 * While the header is written out_base is 0, so every value is printed.
 * Afterwards out_base is 256: the encoder shifts bits into out_value behind a
 * leading 1 (the printf() return value of a single character), so the byte is
 * printed exactly when 8 new bits have arrived.  The decoder stores
 * pixel + 256 in out_value to print a pixel.
 */
static long out_value = 'P';
static long out_base = 0;
static long print_mode;  /* < 1: print raw bytes, otherwise decimal text  */
static long io_mode;     /* header: 15 - magic;  later: 0 = decompress,
                            -1 = compress raw, -2 = compress ASCII image  */

/* The separator after a decimal value is a newline when tag % 8 == 0 */
static void flush_output(long tag) {
  if (out_value >= out_base) {
    if (print_mode < 1)
      out_value = printf("%c", (int)(out_value - out_base));
    else
      out_value = printf("%ld%c", out_value - out_base, tag & 7 ? ' ' : '\n');
  }
}

/* Read a decimal number from a PNM header/body, skipping '#' comments */
static long read_number(void) {
  char hash[2];
  long number = 0;

  while (scanf(" %1[#]%*[^\n]", hash) != 0)
    ;
  scanf("%ld%*c", &number);
  return number;
}

/* Print any waiting value, then read either a number or a single byte */
static long read_input(long tag) {
  flush_output(tag);
  return io_mode / 2 ? read_number() : getchar();
}

/* Sum of the weights .9 * .7^i for i = 0 .. n-1 */
static double weight_sum(long n) {
  double sum = 0;

  while (n-- > 0)
    sum = .9 + .7 * sum;
  return sum;
}

/* ---------------------------------------------------------------------- */
/* Binary arithmetic coder (bit by bit, 17 bit range [0, 2*HALF))          */
/* ---------------------------------------------------------------------- */

static long low, high;    /* current interval [low, high]                 */
static long code;         /* decoder: code value * 256 (8 bits lookahead) */
static long pending;      /* encoder: 1 + number of postponed bits;
                             decoder: number of shifts so far             */
static int encoding;

/* Append one bit to the output byte and print it when 8 bits are ready */
static void put_bit(long bit) {
  out_value += bit + out_value;
  flush_output(0);
}

/*
 * Narrow the interval to the lower part [low, low + split] (bit 0) or the
 * upper part [low + split + 1, high] (bit 1), then shift out every bit that
 * is already known (renormalisation).
 */
static long code_bit(long split, long bit) {
  if (bit)
    low = low + split + bit;
  else
    high = low + split + bit;

  for (;;) {
    long out_bit, scale, subtract, tag;

    out_bit = scale = low / HALF;
    if (out_bit == 0 && high >= HALF) {
      /* Underflow: the interval straddles the middle */
      scale = 2 * low / HALF;
      if (!scale || 2 * high / HALF >= 3)
        break;
    }
    /* 2*HALF: both in upper half, 0: both in lower half, HALF: middle */
    subtract = HALF * (out_bit + scale);

    if (encoding) {
      /* Write the known bit followed by the postponed opposite bits */
      while (pending-- && out_bit == scale) {
        put_bit(out_bit);
        out_bit = scale = !subtract;
      }
    } else {
      pending--;
    }
    low += low - subtract;
    pending += 2;

    /* The decoder shifts in a new byte after every 8 bits */
    tag = !encoding && pending % 8 == 0;
    flush_output(tag);
    code += code - subtract * BYTE_BASE + (tag ? getchar() : 0);

    high += high - subtract + 1;
  }
  return 1;
}

/* ---------------------------------------------------------------------- */
/* Statistics                                                              */
/* ---------------------------------------------------------------------- */

/* Unnormalised cumulative Student t distribution (series expansion) */
static double student_cdf(double u) {
  double k = u * u / DEGREES + 1;
  double sum = 0;
  long n;

  for (n = DEGREES + 1; n >= 2; n -= 2)
    sum = sum * (n - 1) / n / k + 1 / sqrt(k);
  return sum * u;
}

/* out = a + 0.8 * b, for whole 13x13 matrices */
static void blend(double *out, const double *a, const double *b) {
  int i;

  for (i = MATRIX - 1; i >= 0; i--)
    out[i] = a[i] + PRED_DECAY * b[i];
}

/*
 * Weighted least squares prediction.
 *
 * m holds the 12x12 correlations of the neighbours in rows/columns 0..11 and
 * their correlation with the pixel in column 12.  A ridge term `bias` pulls
 * the solution towards the plain average of the neighbours.  Row 12 is set
 * to -neighbours, so after Gaussian elimination of the first 12 columns the
 * bottom right element is neighbours . (A^-1 * b), the predicted value.
 */
static double predict(double *m, const double *neighbour, double bias) {
  int k, i, col;

  for (k = 0; k < NEIGHBOURS; k++) {
    m[k * DIM + NEIGHBOURS] += bias / 12;
    m[k * DIM + k] += bias;
    m[NEIGHBOURS * DIM + k] -= neighbour[k];
    for (i = NEIGHBOURS; i > k; i--) {
      m[i * DIM + k] /= m[k * DIM + k];
      for (col = NEIGHBOURS; col > k; col--)
        m[i * DIM + col] -= m[k * DIM + col] * m[i * DIM + k];
    }
  }
  return m[NEIGHBOURS * DIM + NEIGHBOURS];
}

/* Neighbour positions: {dx in samples of the same colour, dy in rows} */
static const int neighbour_offset[NEIGHBOURS][2] = {
    {-1, 0},  {-2, 0},  {-3, 0}, {-2, -1}, {-1, -2}, {0, -3},
    {1, -2},  {2, -1},  {1, -1}, {0, -2},  {-1, -1}, {0, -1},
};

/* Progress picture characters, from dark to bright */
static const char shades[] = " ,;lf6D@";

/* ---------------------------------------------------------------------- */

int main(int argc, char **argv) {
  long columns = out_value; /* 80 */
  long width, height, maxval, levels, channels;
  long stride, row, x, i;
  long tolerance = 0; /* bin size: 2 * maximum error + 1 */
  long per_char;      /* samples per progress character */
  long *image, *sample, *display;
  double *vert, *horiz, *left_acc, *system, *err_vert, *err_right;
  double neighbour[DIM];
  double bias;
  static const double zero[MATRIX];

  /* ---- header: "P<15-n>\n<width> <height>\n<maxval>\n" ---- */
  read_input(argc); /* prints 'P', skips 'P' */
  out_value = io_mode = print_mode = 15 - read_number();
  out_value = width = read_input(8);
  out_value = height = read_input(print_mode);
  width *= channels = print_mode % 3 ? 1 : 3; /* PPM: 3 samples per pixel */
  if (!(maxval = out_value = read_input(8)))
    return 0;

  stride = width + 9;
  image = malloc(sizeof(long) * stride * (height + 4));
  display = calloc(stride, sizeof(long));
  vert = calloc(width * MATRIX, sizeof(double));
  horiz = calloc(width * MATRIX, sizeof(double));
  left_acc = calloc(MATRIX, sizeof(double));
  system = calloc(MATRIX, sizeof(double));
  err_vert = calloc(width, sizeof(double));
  err_right = calloc(width, sizeof(double));

  /* Everything outside the image looks like mid grey */
  for (i = stride * (height + 4); i--;)
    image[i] = maxval / 2;
  bias = columns;

  flush_output(0); /* prints maxval */
  out_value = 0;

  out_base = BYTE_BASE;
  print_mode = 4 - print_mode;
  io_mode = print_mode / 4;
  encoding = io_mode != 0;

  /* Start the arithmetic coder (the leading bits are dropped) */
  code_bit(code_bit(169, 0), 0);
  out_value = 1;

  while (--argc) {
    long arg = atoi(*++argv);
    if (arg > 0)
      columns = arg;
    else
      tolerance = -2 * arg;
  }
  per_char = width / columns + 1;
  per_char += channels > per_char;
  levels = maxval + 1;
  tolerance++;

  sample = image + 3 * stride + 1;
  for (row = 0; row < height; row++, sample += 9) {
    double row_err = 0, pred, sigma, acc = 0;
    const double *prev = zero;

    /* Sum the column accumulators from the right end of the row */
    for (x = 0; x < width; x++) {
      blend(horiz + x * MATRIX, vert + x * MATRIX, prev);
      prev = horiz + x * MATRIX;
      acc = ERROR_DECAY * (acc + err_vert[x]);
      err_right[x] = acc;
    }
    for (i = 0; i < MATRIX; i++)
      left_acc[i] = 0;

    /* x counts down: x = width-1 is the leftmost sample of the row */
    for (x = width - 1; x >= 0; x--, sample++) {
      long done = width - 1 - x;
      long value, lo, hi;
      double *col_matrix = vert + x * MATRIX;

      for (i = 0; i < NEIGHBOURS; i++)
        neighbour[i] = sample[neighbour_offset[i][0] * channels +
                              neighbour_offset[i][1] * stride];

      /* sigma from the weighted squared errors of earlier pixels */
      flush_output(-done);
      sigma = weight_sum(done);
      sigma = weight_sum(row) * weight_sum(x + 1) +
              weight_sum(row + 1) * sigma + .01;
      sigma = sqrt((row_err + err_right[x] + .1) / sigma);

      blend(system, left_acc, horiz + x * MATRIX);
      pred = predict(system, neighbour, bias) + .5;

      value = encoding ? read_input(-io_mode) : 0;

      /* Code the value by repeatedly splitting the interval [lo, hi) */
      lo = 0;
      hi = levels;
      while (hi - lo > tolerance) {
        long split = pred + tolerance / 2 + 1;
        long cut, bit;
        double base;

        split %= tolerance;
        split += (hi + lo + tolerance - 2 * split) / 2 / tolerance * tolerance;

#define MASS(v) (student_cdf((v - pred) / sigma) - base + 1e-6 * (v - lo))
        base = student_cdf((lo - pred) / sigma);
        cut = (high - low) * MASS(split) / MASS(hi);
        bit = encoding ? value >= split : code / BYTE_BASE > cut + low;
        code_bit(cut, bit);
        if (bit)
          lo = split;
        else
          hi = split;
      }
      value = (lo + hi) / 2;
      *sample = value;
      neighbour[NEIGHBOURS] = value;
      if (!encoding)
        out_value = value + BYTE_BASE;

      /* ASCII art progress display on stderr */
      {
        long sum = value + display[x / per_char];
        if (x % per_char + row % (per_char * 2 / channels))
          display[x / per_char] = sum;
        else {
          putc(x ? shades[4 * channels * sum / per_char / per_char / levels]
                 : '\n',
               stderr);
          display[x / per_char] = 0;
        }
      }

      /* Add this pixel to its column accumulator */
      for (i = NEIGHBOURS * DIM; i--;)
        col_matrix[i] = neighbour[i % DIM] * neighbour[i / DIM] / sigma +
                        PRED_DECAY * col_matrix[i];

      /* Adapt the bias: would a larger one have predicted better? */
      pred -= value + .5;
      blend(system, left_acc, horiz + x * MATRIX);
      sigma = predict(system, neighbour, bias / .9) - value - pred;

      for (i = 0; i < MATRIX; i++)
        left_acc[i] = PRED_DECAY * left_acc[i] + PRED_DECAY * col_matrix[i];

      err_vert[x] = ERROR_DECAY * err_vert[x] + pred * pred;
      row_err += err_vert[x];
      row_err *= ERROR_DECAY;
      bias += pred > 0 ? -sigma : sigma;
    }
  }

  /* Flush the arithmetic coder */
  code_bit(code_bit(0, 0), 0);
  return 0;
}
