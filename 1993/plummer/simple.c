/*
 * The counter string is argv[1].  Instead of calling strlen(), the program
 * relies on the arguments being stored one after another in memory, so the
 * byte just before the last argument is the NUL terminator of argv[1].
 */
char *digit, *last_digit_end;

int main(int argc, char **argv)
{
  char *number;

  if (argc < 3)
    exit(1);

  number = argv[1];

  /* The NUL that ends the number (just before the last argument). */
  last_digit_end = argv[argc - 1] - 1;

  /* Replace that NUL with a carriage return so each print overwrites the line */
  *last_digit_end = '\r';

  /* Re-terminate the string by blanking out the first char of argv[2] */
  argv[2][0] = '\0';

  for (;;) {
    printf(number);

    /* Increment the number, carrying into more significant digits */
    for (digit = last_digit_end - 1; digit >= number; digit--) {
      if (++*digit <= '9')
        break;
      *digit = '0';
    }
  }
}
