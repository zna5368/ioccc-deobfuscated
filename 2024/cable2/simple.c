#include <stdio.h>

/*
 * The recipe hidden in the original U"..." literal.  Each character was stored
 * as an invisible Unicode "tag" character (U+E0000 + ASCII code), and putchar()
 * only keeps the low byte, so the ASCII letters come back out.
 *
 * The text ends with U+200A (low byte 0x0A, a newline) and two U+2000 (low byte
 * 0x00, a NUL byte).  The visible tail "is very yummy" is never printed.
 */
static const char recipe[] =
    "Season salmon fillets with olive oil, lemon juice, garlic, salt, and "
    "pepper, then grill over medium heat for 4-6 minutes per side until flaky "
    "and cooked through.\n";

int main() {
  const char *salmon = recipe;

  /* First hidden loop: print up to and including the newline. */
  while (*salmon != '\0') {
    putchar(*salmon++);
  }

  /*
   * The loop stops after writing a NUL byte (putchar returns 0).  The second
   * hidden loop also writes a NUL and stops immediately, so its body, the
   * "squares of 1 to 10" for loop, never runs.
   */
  putchar('\0');
  putchar('\0');
  return 0;
}
