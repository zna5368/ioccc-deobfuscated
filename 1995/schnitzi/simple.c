/*
 * The original entry declares no headers and relies on implicit declarations
 * of atoi(), fork() and printf(), so none are included here either.
 *
 * main() is called recursively with argc reused as the candidate divisor.
 * The first call from the C runtime has argc == 2 (program name + number),
 * which conveniently is the first divisor to try.
 */
main(divisor, argv) char **argv;
{
  int number = atoi(argv[1]);
  int not_a_divisor;

  if (divisor > number) {
    /* ran past the number: nothing left to test */
    not_a_divisor = 1;
  } else if (fork()) {
    /* parent: spawn the process that tests the next divisor; printf()
     * returns a positive count, so this is always "true" */
    not_a_divisor = main(divisor + 1, argv);
  } else {
    /* child: test this divisor */
    not_a_divisor = number % divisor;
  }

  /*
   * Divide by 1 to keep the divisor, or by 0 to crash with SIGFPE (core
   * dump) so that non-divisors are never printed.  Integer division by zero
   * is undefined behaviour, so an optimizing compiler may assume the divisor
   * is always 1 and drop the crash entirely; this matches what the original
   * does when built with the same flags.
   */
  divisor /= !not_a_divisor;

  return printf("%d\n", divisor);
}
