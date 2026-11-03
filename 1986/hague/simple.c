/*
 * Morse code tree stored in heap order.  Walking the string from the start,
 * each character gets the next "code" number, beginning at 2.  The binary
 * digits of a code after its leading 1 bit spell the Morse code, 0 = dot and
 * 1 = dash (E = 2 = 10b = ".", T = 3 = 11b = "-", I = 4 = 100b = "..", ...).
 *
 * Lowercase letters are not symbols; they are gaps in the tree.  A lowercase
 * letter uses up its own code number plus (letter - 'a') extra numbers, so
 * 'a' skips 1 code, 'b' skips 2, 'l' skips 12, and so on.
 */
char morse_tree[] = "ETIANMSURWDKGOHVFaLaPJBXCYZQb54a3d2f16g7c8a90l?e'b.s;i,d:";

#define FIRST_CODE 2
#define LINE_SIZE 81

/* write one byte straight to standard output (no stdio buffering) */
void put_char(char c) { (void)write(1, &c, 1); }

/* return the Morse code number of ch, or 0 if it has no Morse code */
int morse_code(char ch) {
  char *entry;
  int code = FIRST_CODE;

  /* convert lowercase input to uppercase */
  if (ch >= 'a' && ch <= 'z')
    ch = ch - 'a' + 'A';

  for (entry = morse_tree; *entry; entry++, code++) {
    if (*entry == ch)
      return code;
    if (*entry >= 'a')
      code += *entry - 'a'; /* skip unused positions in the tree */
  }
  return 0;
}

/*
 * Print the dots and dashes of a code, most significant element first.
 * The original program also writes a NUL byte before every symbol.
 */
void print_morse(int code) {
  if (code > 3)
    print_morse(code >> 1);
  else
    put_char('\0');
  put_char(code & 1 ? '-' : '.');
}

int main() {
  static char line[LINE_SIZE];
  char *p;
  int code;

  while (fgets(line, LINE_SIZE, stdin) == line) {
    line[strlen(line) - 1] = '\0'; /* chop the newline */

    for (p = line; *p; p++) {
      code = morse_code(*p);
      if (code)
        print_morse(code);
      else
        put_char('?');
      put_char(' ');
    }
    put_char('\n');
  }
}
