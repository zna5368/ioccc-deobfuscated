#include <stdio.h>
#include <stdlib.h>

/*
 * Count the occurrences of "goto" in the file named on the command line.
 */
int main(int argc, char *argv[])
{
  char line[80];
  char *pos;
  FILE *file;
  int count = 0;
  int remaining;

  if (argc == 1)
    exit(1);

  file = fopen(argv[1], "r");
  if (!file)
    exit(1);

  while (fgets(line, 79, file)) {
    /*
     * Only start positions that leave more than 4 characters
     * (including the newline) are checked, so a "goto" at the very
     * end of a line without a newline is not counted.
     */
    remaining = strlen(line) - 4;
    for (pos = line; remaining > 0; pos++, remaining--) {
      if (!memcmp(pos, "goto", 4)) {
        /* skip over the rest of the word */
        pos += 3;
        remaining -= 3;
        count++;
      }
    }
  }
  fclose(file);

  printf("%d    goto   \'s\n", count);
  exit(0);
}
