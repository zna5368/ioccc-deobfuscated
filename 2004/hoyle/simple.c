#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_COEFFICIENTS 8
#define X_MIN -32 /* leftmost column */
#define X_MAX 31  /* rightmost column */
#define ROWS 32   /* rows in the graph */

double coefficients[MAX_COEFFICIENTS];

/* evaluate a0*x^0 + a1*x^1 + ... + a7*x^7 */
double polynomial(int x) {
  int power;
  double sum = 0;
  for (power = 0; power < MAX_COEFFICIENTS; ++power)
    sum += coefficients[power] * pow(x, power);
  return sum;
}

/*
 * choose the character for a point that lies inside the current row,
 * based on where its neighbours lie relative to the row
 */
char curve_char(int x, double top, double bottom) {
  if (polynomial(x - 1) > top) /* coming down from above */
    return polynomial(x + 1) > top ? 'V' : '\\';
  if (polynomial(x - 1) > bottom) /* coming from inside this row */
    return polynomial(x + 1) > top ? '/' : '_';
  /* coming up from below */
  if (polynomial(x + 1) > top)
    return '/';
  if (polynomial(x + 1) > bottom)
    return '_';
  return '^';
}

int main(int argc, char *argv[]) {
  int i, x, row;
  double scale = 0, top, bottom;

  /* read up to 8 coefficients, missing ones are zero */
  for (i = 1; i <= MAX_COEFFICIENTS; i++)
    coefficients[i - 1] = argc > i ? atof(argv[i]) : 0;

  /* find the largest magnitude (truncated to int) for y auto scaling */
  for (x = X_MIN; x <= X_MAX; x++)
    if (abs((int)polynomial(x)) > scale)
      scale = abs((int)polynomial(x));

  /* each row covers the y range (bottom, top] */
  bottom = scale;
  for (row = -ROWS / 2; row < ROWS / 2; row++) {
    top = bottom;
    bottom = top - 2 * scale / ROWS;

    for (x = X_MIN; x <= X_MAX; x++) {
      double y = polynomial(x);
      char c;
      if (top >= y && y > bottom)
        c = curve_char(x, top, bottom);
      else if (x == 0)
        c = '|'; /* y axis */
      else if (row == 0)
        c = '_'; /* x axis */
      else
        c = ' ';
      putchar(c);
    }
    putchar('\n');
  }
  return 0;
}
