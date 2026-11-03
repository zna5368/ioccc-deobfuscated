#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <float.h>
#include <iso646.h>
#include <signal.h>
#include <sys/wait.h>

#define KILOBYTE 1024UL
#define THRASH_SECONDS 5

/* 'm' for malloc or 'c' for calloc, printed as "%calloc" */
static char alloc_letter = 'm';

/* print a size split into megabytes, kilobytes and bytes */
static void print_size(char *verb, unsigned long size)
{
    printf("%s to %calloc %luM+%luK+%lu bytes", verb, alloc_letter,
           size >> 20, (size & 0xffc00) >> 10, size & 1023);
}

static char *allocate(size_t size)
{
    if (alloc_letter == 'c')
        return calloc(size, 1);
    return malloc(size);
}

/* child: find the largest allocation and touch every page of it */
static int child_test(void)
{
    size_t size = 0;
    size_t bit = ~(size_t)0 ^ (~(size_t)0 >> 1); /* highest bit only */
    char *memory;
    size_t offset;

    /* build the largest allocatable size one bit at a time */
    for (; bit != 0; bit >>= 1) {
        size |= bit;
        print_size("Trying", (unsigned long)size);
        memory = allocate(size);
        if (memory != NULL) {
            printf("..YEP\n");
            free(memory);
        } else {
            printf("..nope\n");
            size &= ~bit;
        }
    }

    memory = allocate(size);
    assert(memory != NULL);
    print_size("Able", (unsigned long)size);
    putchar('\n');

    /* write one byte in every kilobyte; a slow write means thrashing */
    for (offset = 0; offset < size; offset += KILOBYTE) {
        time_t start = time(NULL);
        memory[offset] = '\a';
        printf("\r%lu kilo bytes ok ", (unsigned long)(1 + (offset >> 10)));
        if (difftime(time(NULL), start) - THRASH_SECONDS > FLT_EPSILON) {
            free(memory);
            return 1;
        }
    }
    free(memory);
    return 0;
}

/* parent: wait for the child and explain how it ended */
static int report_child(void)
{
    int status;

    wait(&status);
    if (WIFSIGNALED(status)) {
        int signal_number = WTERMSIG(status);
        printf(" %calloc lied! Killed by SIG", alloc_letter);
        switch (signal_number) {
        case SIGKILL:
            puts("KILL");
            break;
        case SIGSEGV:
            puts("SEGV");
            break;
        default:
            printf("NAL %d\n", signal_number);
        }
    } else if (WEXITSTATUS(status) == 1) {
        printf(" %calloc lied! Thr\aashing\n", alloc_letter);
    } else {
        printf("no overcommit\n");
    }
    return status;
}

int main(int argc, char **argv)
{
    pid_t pid;

    /* the last argument (or program name) chooses calloc */
    if (argv[argc - 1][0] == 'c')
        alloc_letter = 'c';

    setbuf(stdout, NULL);
    pid = fork();
    if (pid == 0)
        return child_test();
    if (pid == -1)
        assert(0);
    return report_child();
}
