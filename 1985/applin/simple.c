/*
 * Prints "Hello, world!" one character per process: each run prints the
 * first character of its message, then re-executes itself with the rest of
 * the message as its only argument.  When only one character is left (the
 * extra trailing newline) the chain stops.
 */
int main(int argc, char **argv)
{
    char **message;
    int countdown = argc;

    /* On the first run argc is 1, so argv[1] becomes the greeting.
     * On later runs argv[1] is the remaining message passed by the parent. */
    argv[argc] = strdup("Hello, world!\n\n");
    message = argv + 1;

    /* Keep going while the message has more than one character left */
    while ((*message)[1] != '\0') {
        if (countdown-- == 0) {
            /* Run this program again with the message minus its first
             * character.  execlp only returns if it failed. */
            message = argv;
            if (execlp(argv[0], argv[0], argv[1] + 1, (char *)0) == 0)
                break;
        }

        /* Print the first character (only the first pass sees it,
         * because it is erased right afterwards) */
        write(1, *message, **message != '\0');
        **message = '\0';
    }
}
