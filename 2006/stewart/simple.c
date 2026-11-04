#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * One affine map of the Iterated Function System:
 *   x' = a*x + b*y + e
 *   y' = c*x + d*y + f
 * chosen with probability p.
 */
struct transform {
  double a, b, c, d, e, f;
  double probability;
};

/* the IFS file: a header followed by the list of maps */
struct ifs {
  long count;
  double x_min, y_min, x_max, y_max;
  struct transform maps[];
};

/* xorshift pseudo random number generator */
static long seed = 8675309;

static long next_random(void) {
  long previous = seed;
  seed ^= seed << 11;
  seed ^= seed >> 15;
  seed ^= seed << 3;
  return (previous + seed) & 0x7fffffff;
}

int main(int argc, char **argv) {
  long size, iterations, bitmap_bytes, i, n;
  unsigned char *bitmap;
  struct ifs *ifs;
  struct transform *t;
  FILE *fp;
  double x = 0, y = 0, r, nx, ny;

  if (argc != 4)
    printf("bad args\n"), exit(1);

  size = atoi(argv[1]);
  iterations = atoi(argv[2]);

  /* one bit per pixel */
  bitmap_bytes = size * size / 8;
  bitmap = malloc(bitmap_bytes);
  memset(bitmap, 0, bitmap_bytes);

  /* read the IFS description */
  fp = fopen(argv[3], "r");
  fscanf(fp, "%ld", &n);
  ifs = malloc(sizeof(struct ifs) + n * sizeof(struct transform));
  ifs->count = n;
  fscanf(fp, "%lf %lf %lf %lf", &ifs->x_min, &ifs->y_min, &ifs->x_max,
         &ifs->y_max);
  for (i = 0; i < ifs->count; ++i) {
    t = &ifs->maps[i];
    fscanf(fp, "%lf %lf %lf %lf %lf %lf %lf", &t->a, &t->b, &t->c, &t->d,
           &t->e, &t->f, &t->probability);
  }
  fclose(fp);

  /* chaos game: repeatedly apply a randomly chosen map */
  for (i = 0; i < iterations; ++i) {
    r = next_random() / (double)0x7fffffff;

    /* pick a map using the probabilities as weights */
    for (n = 0; n < ifs->count; ++n) {
      if (r < ifs->maps[n].probability)
        break;
      r -= ifs->maps[n].probability;
    }
    t = &ifs->maps[n];

    nx = t->a * x + t->b * y + t->e;
    ny = t->c * x + t->d * y + t->f;
    x = nx;
    y = ny;

    /* skip the first few points until they settle onto the fractal */
    if (i > 10 && x >= ifs->x_min && x <= ifs->x_max && y >= ifs->y_min &&
        y <= ifs->y_max) {
      /* scale into 0..1 */
      double px = (x - ifs->x_min) / (ifs->x_max - ifs->x_min);
      double py = (y - ifs->y_min) / (ifs->y_max - ifs->y_min);
      /* y grows upwards, image rows grow downwards */
      long row = (long)(size - py * size);
      long col = (long)(px * size);
      long pixel = row * size + col;

      bitmap[pixel / 8] |= 1 << (pixel % 8);
    }
  }

  /* write the bitmap as an XBM image (which is C source code) */
  printf("#define x_width %ld\n#define x_height %ld\nstatic char x_bits[] = {\n",
         size, size);
  for (i = 0; i < bitmap_bytes; ++i) {
    if (i)
      printf(",");
    printf("0x%02x", bitmap[i]);
  }
  printf("};\n");

  free(bitmap);
  free(ifs);
  return 0;
}
