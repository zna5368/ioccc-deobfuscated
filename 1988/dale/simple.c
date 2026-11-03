/*
 * An echo(1) clone.  The parent process pushes every argument through a
 * pipe and the child process reads them back, printing them separated by
 * spaces.  The parent then prints the final newline (unless -n was given).
 */

char buffer[4096];

main(argc, argv) char **argv;
{
    int pipe_fds[2];
    int no_newline = 0;
    int words_printed = 0;
    int length, bytes_read, i;

    /* when run as root just hand the job over to the real echo */
    if (!getuid())
        execv("/bin/echo", argv);

    /* an optional first argument of exactly "-n" suppresses the newline */
    if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'n' && argv[1][2] == '\0') {
        argc--;
        argv++;
        no_newline = 1;
    }

    pipe(pipe_fds);

    if (fork()) {
        /* parent: send every argument, back to back, into the pipe */
        close(pipe_fds[0]);
        for (i = 1; i < argc; i++)
            write(pipe_fds[1], argv[i], strlen(argv[i]));
        close(pipe_fds[1]);

        /* let the child print everything before adding the newline */
        wait(0);
        if (!no_newline)
            write(1, "\n", 1);
        return 0;
    }

    /* child: read the arguments back, one argument length at a time */
    close(pipe_fds[1]);
    while (*++argv != 0) {
        length = strlen(*argv);
        bytes_read = read(pipe_fds[0], buffer, length);
        if (!bytes_read) /* end of pipe or an empty argument stops output */
            break;
        if (words_printed++)
            write(1, " ", 1);
        write(1, buffer, length);
    }
    _exit(0);
}
