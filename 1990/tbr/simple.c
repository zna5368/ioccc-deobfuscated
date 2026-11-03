/*
 * A tiny Unix shell: runs commands with arguments, handles pipes (|),
 * input/output redirection (< and >) and the cd builtin.
 *
 * The command line is parsed backwards (right to left).  Each word is
 * pushed onto a stack that grows downwards so that the words end up in the
 * right order for execvp().
 */

#define STACK_TOP 98

/* character classes stored in char_class[] */
#define WORD_CHAR  0  /* part of a word */
#define BLANK      6  /* space or tab */
#define SEPARATOR  9  /* '|' or the '\0' before the line */
#define REDIRECT  32  /* '<' or '>' */

#define PROMPT     -8 /* error code that makes report() print "$ " */

/*
 * The order of these globals matters: the compiler places them in memory in
 * reverse order, so the byte just before line[] is char_class[255] == 0,
 * which acts as the left end sentinel when the line is scanned backwards.
 */
char *cursor, line[512], char_class[256], *args[99], **arg_top, *redirect[3];
int pipe_fds[2], pid;

int report(int status);
int run(int out_fd, int pipe_out);

int main()
{
    char_class[' '] = BLANK;
    char_class['\t'] = BLANK;
    char_class['\0'] = SEPARATOR;
    char_class['|'] = SEPARATOR;
    char_class['<'] = REDIRECT;
    char_class['>'] = REDIRECT;

    for (;;) {
        report(PROMPT);

        cursor = fgets(line, 512, stdin);
        cursor[strlen(cursor) - 1] = '\0';     /* strip the newline */

        /* move the cursor to the end of the line (skipping line[0]) */
        while (*++cursor)
            ;

        run(0, 0);
    }
}

/*
 * Parse and run the command that ends at cursor and starts after the
 * previous '|' (or the start of the line).  If out_fd is not 0 the
 * command's standard output goes to that pipe.  Commands to the left of
 * a '|' are run by calling run() recursively.
 */
int run(int out_fd, int pipe_out)
{
    redirect[0] = redirect[2] = 0;   /* [0] is '<' file, [2] is '>' file */
    arg_top = args + STACK_TOP;      /* args[98] stays NULL */

    while (char_class[*--cursor] != SEPARATOR) {
        if (char_class[*cursor] == WORD_CHAR) {
            /* terminate the word, find its start and push it */
            *++cursor = '\0';
            while (char_class[*--cursor] == WORD_CHAR)
                ;
            *--arg_top = ++cursor;
        }
        if (char_class[*cursor] & REDIRECT) {
            /* '<' & 2 == 0 and '>' & 2 == 2: steal the word to the right */
            redirect[*cursor & 2] = *arg_top;
            if (arg_top != args + STACK_TOP)
                ++arg_top;
        }
    }

    if (arg_top == args + STACK_TOP)    /* empty command */
        return 0;

    if (strcmp(*arg_top, "cd") == 0) {
        /* doubled so that a failure (-2) prints "?" without exiting */
        report(chdir(arg_top[1]) * 2);
        return 0;
    }

    if (*cursor) {                      /* stopped at a '|' */
        pipe(pipe_fds);
        pipe_out = pipe_fds[1];
    }

    if ((pid = fork())) {
        /* parent */
        report(pid);
        if (pipe_out) {
            /* run the command to the left, writing into the pipe */
            run(pipe_out, 0);
            close(pipe_out);
            close(pipe_fds[0]);
        }
        wait(0);
    } else {
        /* child: set up standard input */
        if (pipe_out) {
            dup2(pipe_fds[0], 0);
            close(pipe_fds[0]);
            close(pipe_out);
        } else if (redirect[0]) {
            close(0);
            report(open(redirect[0], 0));
        }

        /* set up standard output */
        if (out_fd) {
            dup2(out_fd, 1);
            close(out_fd);
        } else if (redirect[2]) {
            close(1);
            report(creat(redirect[2], 0666));
        }

        report(execvp(*arg_top, arg_top));
    }
    return 0;
}

/*
 * Print the prompt or an error message.
 *   -8      -> "$ "  (the prompt)
 *   -1      -> "?\n" and exit (failed system call in a sub-shell)
 *   -2      -> "?\n" (failed cd)
 *   >= 0    -> nothing
 */
int report(int status)
{
    if (status < 0) {
        write(2, "?\n$ " + -status / 4, 2);
        if (status == -1)
            exit(1);
    }
    return 0;
}
