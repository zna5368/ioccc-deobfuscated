#include <stdio.h>

#define BYTES_PER_LINE 16

static int is_printable(int c) { return c >= ' ' && c <= '~'; }

int main(int argc, char *argv[]) {
  FILE *file;
  unsigned offset = 0;
  int line[BYTES_PER_LINE];
  int i;

  if (argc != 2) {
    printf("Use:\n\tdump file\n");
    exit(1);
  }
  file = fopen(argv[1], "r");
  if (file == NULL) {
    printf("Unable to open file '%s'\n", argv[1]);
    exit(1);
  }

  for (;;) {
    /* read the next 16 bytes; past the end of file these are all EOF */
    for (i = 0; i < BYTES_PER_LINE; i++)
      line[i] = fgetc(file);
    if (line[0] == EOF)
      break;

    printf("%4x: ", offset);

    /* hex column, blank padding for bytes past the end of file */
    for (i = 0; i < BYTES_PER_LINE; i++) {
      if (line[i] != EOF)
        printf("%2x ", line[i]);
      else
        printf("   ");
    }
    printf("   ");

    /* text column, unprintable bytes are shown as '.' */
    for (i = 0; i < BYTES_PER_LINE; i++) {
      if (line[i] != EOF) {
        if (!is_printable(line[i]))
          line[i] = '.';
        putchar(line[i]);
      } else {
        putchar(' ');
      }
    }
    printf("\n");

    offset += BYTES_PER_LINE;
  }
  return 0;
}
