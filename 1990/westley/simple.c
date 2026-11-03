/*
 * 1990/westley - "Picking the Daisy" - deobfuscated
 *
 * Both answers are cut out of one sentence. Printing it while skipping
 * over parts of it gives either
 *
 *     "loves me\n"        or        "loves me, not\n"
 *
 * Like the original, no headers are included; atoi(), exit(), puts() and
 * putchar() are implicitly declared.
 */

/*
 * The message, starting after "The ". Indices used below:
 *
 *   0         1         2         3         4
 *   0123456789012345678901234567890123456789012345
 *   gloves are OFF this time, I detest you, snot\n\0sed GEEK!
 *    ^^^^^^               ^^^^                ^^^^ ^^
 *    "loves "             "me, "              "not\n\0"
 */
char *message = "The gloves are OFF this time, I detest you, snot\n\0sed GEEK!";

#define SKIP_AFTER_SPACE 15  /* jump from a space to just before the next word */
#define SKIP_TO_NEWLINE 20   /* jump from ',' in "me, " to the '\n' */

main(argc, argv) int argc;
char **argv;
{
    char *text = message + 4;  /* skip "The " */
    int petals_left;           /* answers still to print after this one */
    int loves_me = 1;          /* the next answer is "loves me" */
    int pos;

    if (atoi(argv[1]) < 1)
        exit(1);

    petals_left = atoi(argv[1]) - 1;

    do {
        /* whoever is being asked about: the program's own name */
        puts(argv[0]);

        for (pos = 1; text[pos - 1] != '\0'; pos++) {
            /* two characters before the 'I' is the ',' after "me" */
            if (text[pos + 2] == 'I') {
                /*
                 * The original tests the first character of argv[3]. With
                 * the single expected argument that is argv[argc + 1], one
                 * past the NULL ending argv, which on Unix is the first
                 * environment string. Unless that string starts with 'I',
                 * "loves me" jumps straight to the '\n'.
                 */
                if (loves_me && argv[argc + 1][0] != 'I')
                    pos += SKIP_TO_NEWLINE;
                loves_me = !loves_me;
            }

            /* also prints the '\0' that ends "snot\n" */
            putchar(text[pos]);

            if (text[pos] == ' ')
                pos += SKIP_AFTER_SPACE;
        }
    } while (petals_left--);

    exit(pos);  /* one past the '\0', i.e. 46 */
}
