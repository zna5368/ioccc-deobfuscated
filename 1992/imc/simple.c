#include <stdio.h>
#include <stdlib.h>

/*
 * Mandelbrot / Julia set drawer.
 *
 * Writes a monochrome Sun raster file to stdout, or with -text a picture made
 * of characters.  One dot per pixel row is written to stderr as progress.
 */

/* Characters for up to 16 shades of grey, from blank up to a star. */
#define SHADES " .:';+<?F7RQ&%#*"

/* Sun raster file magic number. */
#define RAS_MAGIC 0x59A66A95

/* Picture settings, changed by the command line options. */
static double centre_re = 0, centre_im = 0; /* -centre x y */
static double scale = 0;                    /* -factor f (pixels per unit) */
static double c_re = 0, c_im = 0;           /* -julia x y (constant added) */
static int julia_mode = 0;
static int iteration_limit = 128;           /* -limit l */
static int colour_mask = 1;                 /* -mask m */
static int width = 768, height = 768;       /* -size x y */
static int text_width = 63, text_height = 23;
static int text_mode = 0;                   /* -text */

/*
 * Sun raster header: magic, width, height, depth, length, type, maptype,
 * maplength.  It is an array of long but only 32 bytes of it are written,
 * exactly as the original program does.
 */
static long raster_header[8] = {RAS_MAGIC, 256, 192, 1, 6912, 1, 0, 0};

static void zero_error(void)
{
  fputs("0?\n", stderr);
  exit(1);
}

/*
 * Read one number for an option.  The number is taken from the text after the
 * option letter, or else from the next argument if that is a number.
 */
static void read_one(char *rest, const char *fmt, void *value, int argc,
                     char **argv, int *index)
{
  if (sscanf(rest, fmt, value) == 1)
    return;
  if (*index + 1 < argc && sscanf(argv[*index + 1], fmt, value) == 1)
    ++*index;
}

/*
 * Read up to two numbers for an option.  They may follow the option letter or
 * be in the next argument(s), either joined by a comma or as separate
 * arguments.  Arguments that do not start with a number are left alone so
 * they can be read as the next option.
 */
static void read_two(char *rest, const char *pair_fmt, const char *one_fmt,
                     void *first, void *second, int argc, char **argv,
                     int *index)
{
  int got = 0;

  if (*rest)
    got = sscanf(rest, pair_fmt, first, second);

  if (got == 0) {
    if (*index + 1 >= argc)
      return;
    got = sscanf(argv[*index + 1], pair_fmt, first, second);
    if (got == 0)
      return;
    ++*index;
  }

  /* only the first number was found so far: look for the second one */
  if (got < 2 && *index + 1 < argc &&
      sscanf(argv[*index + 1], one_fmt, second) != 0)
    ++*index;
}

static void parse_options(int argc, char **argv)
{
  int index;

  for (index = 1; index < argc; index++) {
    char *option = argv[index];

    /* skip a leading '-' (or any other character before '0') */
    if (option[0] && option[0] < '0')
      option++;

    switch (option[0]) {
    case 'c': /* centre x y */
      read_two(option + 1, "%lg,%lg", "%lg", &centre_re, &centre_im, argc,
               argv, &index);
      break;

    case 'f': /* factor f */
      read_one(option + 1, "%lg", &scale, argc, argv, &index);
      break;

    case 'j': /* julia x y */
      read_two(option + 1, "%lg,%lg", "%lg", &c_re, &c_im, argc, argv,
               &index);
      julia_mode = 1;
      break;

    case 'l': /* limit l */
      read_one(option + 1, "%d", &iteration_limit, argc, argv, &index);
      if (!iteration_limit)
        zero_error();
      break;

    case 'm': /* mask m */
      read_one(option + 1, "%d", &colour_mask, argc, argv, &index);
      if (!colour_mask)
        zero_error();
      break;

    case 's': /* size x y */
      read_two(option + 1, "%d,%d", "%d", &width, &height, argc, argv,
               &index);
      if (!width || !height)
        zero_error();
      /* a later -text uses this size instead of its own default */
      text_width = width;
      text_height = height;
      break;

    case 't': /* text */
      text_mode++;
      width = text_width;
      height = text_height;
      break;

    default:
      fprintf(stderr, "%s?\n", argv[index]);
      exit(1);
    }
  }
}

int main(int argc, char **argv)
{
  char *bitmap;
  long bitmap_size, byte;
  int pad_byte, bit, row, col, iterations, shade;
  double left, top, im, z_re, z_im, re2, im2;

  parse_options(argc, argv);

  if (!scale)
    scale = width / 4.;

  /* raster rows are padded to a multiple of 16 bits */
  pad_byte = ((width - 1) & 15) < 8;
  bitmap_size = (long)(((width + 7) >> 3) + pad_byte) * height;
  bitmap = malloc(bitmap_size);
  if (!bitmap) {
    perror("malloc");
    exit(1);
  }

  /* complex coordinates of the top left corner */
  left = centre_re - (width / 2) / scale;
  top = centre_im + (height / 2) / scale;

  byte = 0;
  bit = 0x80;
  for (row = 0; row < height; row++) {
    im = top - row / scale;
    if (!julia_mode)
      c_im = im;
    if (text_mode)
      putc('\n', stdout);
    putc('.', stderr);

    for (col = 0; col < width; col++) {
      if (julia_mode) {
        /* Julia: start at the pixel, add the fixed constant */
        z_re = col / scale + left;
        z_im = im;
      } else {
        /* Mandelbrot: start at zero, add the pixel */
        c_re = col / scale + left;
        z_re = z_im = 0;
      }

      /* iterate z = z * z + c until it escapes or the limit is reached */
      iterations = 0;
      while (iterations < iteration_limit &&
             (re2 = z_re * z_re) + (im2 = z_im * z_im) < 4.) {
        z_im = 2 * z_re * z_im + c_im;
        z_re = re2 - im2 + c_re;
        iterations++;
      }

      shade = iterations & colour_mask;
      if (text_mode)
        putc(SHADES[15 * shade / colour_mask], stdout);

      /* a set bit is black in a Sun raster */
      if (shade)
        bitmap[byte] &= ~bit;
      else
        bitmap[byte] |= bit;
      bit >>= 1;
      if (!bit) {
        bit = 0x80;
        byte++;
      }
    }

    /* finish the partly filled byte and add the padding byte */
    if (bit != 0x80) {
      bit = 0x80;
      byte++;
    }
    if (pad_byte)
      bitmap[byte++] = 0;
  }

  if (text_mode)
    putc('\n', stdout);
  putc('\n', stderr);

  raster_header[1] = width;
  raster_header[2] = height;
  raster_header[4] = bitmap_size;
  if (!text_mode) {
    fwrite(raster_header, 1, 32, stdout);
    fwrite(bitmap, 1, bitmap_size, stdout);
  }
  free(bitmap);
  exit(0);
}
