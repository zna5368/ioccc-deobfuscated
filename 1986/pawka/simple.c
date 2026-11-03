#include "stdio.h"

/*
 * Each row of the banner is run-length encoded as pairs of
 * (repeat count digit, character to print).  The final '?' of
 * every row ends the line.
 */
char *banner[] = {
    "4 5O2 4B3 5F2 1U3 1U2 5S2 5C2 5A2 5T2 5E2 4D3 4?",
    "4 1O3 1O2 1B3 1B2 1F6 1U3 1U2 1S6 1C6 1A3 1A4 1T4 1E6 1D3 1D6 1?",
    "4 1O3 1O2 5B2 3F4 1U3 1U3 4S2 1C6 5A4 1T4 5E2 1D3 1D6 1?",
    "4 1O3 1O2 1B3 1B2 1F6 1U3 1U6 1S2 1C6 1A3 1A4 1T4 1E6 1D3 1D4 1?",
    "4 5O2 4B3 1F6 5U2 5S2 5C2 1A3 1A4 1T4 5E2 4D5 1?",
    NULL};

char *pos;
int count, row;

main() {
  for (row = 0; banner[row] != NULL; row++) {
    pos = banner[row];
    while (*pos) {
      count = *pos++ - '0';
      while (count--)
        putchar(*pos);
      if (*pos == '?')
        putchar('\n');
      pos++;
    }
  }
}
