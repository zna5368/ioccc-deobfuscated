/*
 * Only the first character of the input decides the result: a real pipe
 * character '|' "can be stuffed with data", anything else is just a
 * picture of a pipe.
 */

static char *picture_of_pipe =
    "|\\_/|\n"
    "|   |           /\n"
    "\\==============/\n";

static char *real_pipe = "       |\n";

int main()
{
    int is_pipe = getchar() == '|';

    if (is_pipe) {
        printf("%s", real_pipe);
        printf("This is a pipe\n");
    } else {
        printf("%s", picture_of_pipe);
        printf("This is not a pipe\n");
    }

    return 0;
}
