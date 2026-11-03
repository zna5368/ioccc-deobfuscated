/*
 * Estimate pi by measuring the area of a circle drawn in the source code.
 *
 * Every '_' in the picture below is one unit square of the circle.
 * The area is the number of squares and the diameter is the number of rows.
 * Since area = pi * (diameter / 2)^2, pi = 4 * area / diameter^2.
 */

static const char *circle[] = {
    "            _-_-_-_",
    "       _-_-_-_-_-_-_-_-_",
    "    _-_-_-_-_-_-_-_-_-_-_-_",
    "  _-_-_-_-_-_-_-_-_-_-_-_-_-_",
    " _-_-_-_-_-_-_-_-_-_-_-_-_-_-_",
    " _-_-_-_-_-_-_-_-_-_-_-_-_-_-_",
    "_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_",
    "_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_",
    "_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_",
    "_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_",
    " _-_-_-_-_-_-_-_-_-_-_-_-_-_-_",
    " _-_-_-_-_-_-_-_-_-_-_-_-_-_-_",
    "  _-_-_-_-_-_-_-_-_-_-_-_-_-_",
    "    _-_-_-_-_-_-_-_-_-_-_-_",
    "        _-_-_-_-_-_-_-_",
    "            _-_-_-_",
};

float area = 0, diameter = 0;

void measure_circle(void) {
  int row;
  const char *p;

  for (row = 0; row < (int)(sizeof(circle) / sizeof(circle[0])); row++) {
    diameter++;
    for (p = circle[row]; *p; p++) {
      if (*p == '_') {
        area++;
      }
    }
  }
}

int main(void) {
  measure_circle();
  printf("%1.3f\n", 4. * area / diameter / diameter);
  return 0;
}
