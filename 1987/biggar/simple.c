/* Copy standard input to standard output, removing carriage returns */
main()
{
    char c;

    while (read(0, &c, 1) > 0) {
        if (c != '\r') {
            write(1, &c, 1);
        }
    }
}
