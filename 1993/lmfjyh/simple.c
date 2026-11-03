/*
 * The original source file only contains:
 *
 *     char*_=__FILE__;
 *
 * and is compiled under the file name:
 *
 *     ";main(){puts("Hello World!");}char*C=".c
 *
 * Old compilers pasted the file name into __FILE__ without escaping the
 * quotes, so the compiler actually saw the program written out below.
 */

/* the empty string before the first quote in the file name */
char *file_name_prefix = "";

int main(void)
{
    puts("Hello World!");
    return 0;
}

/* the leftover ".c" extension at the end of the file name */
char *file_name_suffix = ".c";
