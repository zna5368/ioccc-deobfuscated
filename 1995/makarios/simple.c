/*
 * Print every prime number whose base 8 (octal) representation is a
 * palindrome, written in octal.  The original has no #include lines and
 * relies on printf being implicitly declared.
 */

#define BASE 8

/*
 * Reverse the low digits of number into a second value until the reversed
 * part is at least as large as what remains.  At that point the number is a
 * palindrome if the two halves match: exactly for an even digit count, or
 * after dropping the middle digit for an odd digit count.
 */
int is_octal_palindrome(int number)
{
    int remaining = number;
    int reversed = 0;
    int palindrome;

    do {
        reversed = reversed * BASE + remaining % BASE;
        remaining /= BASE;
        palindrome = reversed == remaining || reversed / BASE == remaining;
    } while (reversed < remaining);

    return palindrome;
}

/* Plain trial division by every candidate divisor below number. */
int is_prime(int number)
{
    int divisor;

    for (divisor = 2; divisor < number; divisor++) {
        if (number % divisor == 0)
            return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    int number;

    (void)argv;

    /* argc is 1 with no arguments, so counting starts at 2. */
    for (number = argc + 1; number != 0; number++) {
        if (is_octal_palindrome(number) && is_prime(number))
            printf("%o\n", number);
    }
    return 0;
}
