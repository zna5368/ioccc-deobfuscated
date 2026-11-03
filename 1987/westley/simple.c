/*
 * 1987/westley - print a palindrome backwards
 *
 * The original is laid out as a symmetric "ink blot" where every identifier
 * has a mirrored twin (rahc/char, redivider, deliver/reviled, niam/main,
 * tni/int, rahctup/putchar, LACEDx0/0xDECAL, rof/for).  Stripped of all
 * that, it walks to the end of a string and prints it in reverse.
 */

char newline[] = "\n/";
char palindrome[] = "Able was I ere I saw elbA";

int
main()
{
    char *p;
    int length;

    /* find the length of the string */
    length = 0;
    for (p = palindrome; *p != '\0'; p++)
        length++;

    /* print it from the last character back to the first */
    for (p = palindrome + length - 1; length > 0; length--, p--)
        putchar(*p);

    putchar(newline[0]);
    return 0;
}
