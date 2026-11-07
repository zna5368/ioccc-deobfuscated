#include<stdio.h>
/*
 * A poor man's "patch": reads a unified diff on stdin and applies every
 * hunk to the file named on the "--- file" line (directories are stripped).
 *
 * The original prog.c also contains a dormant second stage.  It sits inside
 * a comment that only closes once prog.c has patched itself with the diff
 * at the end of its own source.  Here that stage is enabled by compiling
 * with -DSELF_PATCHED instead.
 */

#define LINE_MAX_LEN   997          /* longest line fgets will read          */
#define NAME_OFFSET    499          /* scratch area for the "--- name" token */
#define TEXT_OFFSET    997          /* where the patched file text is built  */
#define ENGINE_WIDTH   50           /* characters per emitted engine line    */

FILE *file;                         /* file being patched (NULL = none yet)  */
int file_line;                      /* lines consumed from the file so far   */
int old_start, old_count, new_count;
char *self_name = __FILE__;
char *got_line;                     /* result of the last fgets()            */
char buffer[9 << 18];               /* buffer[0..] holds the target filename */
char *cursor = buffer;              /* end of the text built so far          */

/*
 * Source code of the next stage of the quine chain (the git-am patch
 * generator that eventually spells out a diff program).  The original
 * stores it with '"' as W, ' ' as X, '\' as Y and pads it with spaces
 * to draw the shape of the source.
 */
const char *diff_engine =
    "/*    THE DIFFERENCE ENGINE    */#include<stdio.h>"
    "#define Q(m)M[9973],p=9,i,j,k,w,n,T;char*y=#m,*x,m"
    "int Q(B[1999][997];void(S)(){for(j=5;--j;n=j[M]=(j"
    "%2?*M-n/T:T-n%T)-M[j+2])n=M[j<3?p-3:k]/3;x?printf("
    "\"--- %s\\n+++ %s\\n\",x,y):0;for(printf(\"@@ -%d,%d +\\"
    "%d,%d @@\\n\",4[M],2[M],3[M],n);k<p-3;printf(\"%c%s\","
    "\" -+\"[n%3],B[n%3>1?*M-n/3/T+w:T-n/3%T]))n=k++[M];x"
    "=0;}int(main)(int(C),char**V){j=w=997;T='C'-41;if("
    "T){for(;n<108;n++)B[n/51+1][n%51]=\"/* git log --p\\"
    "retty=format:%s */ #include<stdio.h>\\n#define Q(m\\"
    ")M[9973],p=9,i,j,k,w,n,T;char*y=#m,*x,m\\nint Q(\"[n"
    "];for(x=6+3[B];*y;*y/33+k?*x++=50>j?k^=34==*y,48<j"
    "&&k?92:*y++:50/j*10:*y++)j=(x-*B)%w;for(y=41+8[B],"
    "*x=*y=')';x[w*w]=*x,--x>*B;);1<T?y[w*w]=~-(printf("
    "\"From: diff@example.com\\nDate: 2021-01-01T00:00:%\\"
    "02d\\nSubject: %s\\n\",26-T,T[B-1]),*y+=T):0;j=T=26;x"
    "=y=__FILE__;}else{FILE*F;if(C<3)return!!printf(\"u\\"
    "sage: %s FILE1 FILE2\\n\",*V);for(;n<2;n++)for(x=y,F"
    "=fopen(y=*++V,\"r\");fgets(B[n?++j:++T],w,F););}for("
    ";++i<T;)p++[M]=i*5+1;for(*M=j%=w;--j;C=0)for(i=T,n"
    "=p[M]=1[p-T+M]/5*5+7;k=++p,--i;p[M]=C<0?n=p[M+~T]/"
    "5*5:n)for(k=p[M-T],n=n>k+2?k/5*5+7:n/5*5+6,C=w;C--"
    "&&C[i[B]]==C[w[B+j]];);for(i=p-9;i;i-=n?C=1,n<2?1:"
    "T:(C+=C%2?C>10?S(),k=p-3,6-C:2:C>4?k++,0:2,T+1))M["
    "p++]=(n=8[i+M]%5)+3*i;p+=C>6?6-C/2:3;if(C%2)S();})";

/* Read one line at the cursor, from the target file or from the patch. */
void read_line(int from_file)
{
    if (from_file) {
        file_line++;
        got_line = fgets(cursor, LINE_MAX_LEN, file);
    } else {
        got_line = fgets(cursor, LINE_MAX_LEN, stdin);
    }
}

/* Keep the line just read: move the cursor past it. */
void keep_line(void)
{
    while (*cursor)
        cursor++;
}

#ifdef SELF_PATCHED
/* Load our own source into the buffer with every line prefixed by '-'. */
void read_own_source(void)
{
    for (file = fopen(self_name, "r"); fgets(cursor + 1, LINE_MAX_LEN, file);
         file_line++) {
        *cursor = '-';
        cursor++;
        keep_line();
    }
}
#endif

/* Stage two: print a diff that replaces our source with the diff engine. */
int print_replacement_patch(void)
{
    const char *src = diff_engine;
    int column;

    printf("--- %s\n+++ %s\n@@ -1,%d +1,25 @\n%s",
           self_name, self_name, file_line, buffer);

    for (cursor = buffer; *src; ) {
        for (column = 0; column <= ENGINE_WIDTH + 1 && *src; column++) {
            if (column == 0)
                *cursor++ = '+';
            else if (column == ENGINE_WIDTH + 1)
                *cursor++ = '\n';
            else
                *cursor++ = *src++;
        }
    }
    *cursor = 0;
    puts(buffer);
    return 1;
}

/* Copy the rest of the target file and write the result back over it. */
void finish_file(void)
{
    cursor[fread(cursor, 1, 9 << 17, file)] = 0;
    file = freopen(buffer, "w", file);
    fprintf(file, "%s", buffer + TEXT_OFFSET);
    fclose(file);
}

/* Remember the new filename without any leading directories. */
void set_filename(void)
{
    char *dst = buffer, *src = buffer + NAME_OFFSET;
    char c;

    while ((c = *dst = *src++) != 0)
        dst = (c == '/') ? buffer : dst + 1;
}

int main()
{
    int sign;

#ifdef SELF_PATCHED
    read_own_source();
#endif
    if (file)
        return print_replacement_patch();

    for (;;) {
        cursor = buffer + TEXT_OFFSET;

        /* Read patch lines until the next "--- file" header or EOF. */
        for (read_line(0); got_line && !sscanf(cursor, "--- %s",
                                               buffer + NAME_OFFSET);
             read_line(0)) {
            if (!*buffer || sscanf(cursor, "@@ -%d,%d +%d,%d", &old_start,
                                   &old_count, &new_count, &new_count) <= 3)
                continue;

            if (!file) {
                printf("patching file %s\n", buffer);
                file = fopen(buffer, "r");
            }

            /* Copy the untouched lines before the hunk. */
            while (file_line + 1 < old_start) {
                read_line(1);
                keep_line();
            }

            /* Apply the hunk body. */
            while (old_count + new_count) {
                sign = getchar();
                read_line(0);
                if (sign == '-') {          /* drop the next file line   */
                    read_line(1);
                    old_count--;
                } else if (sign == ' ') {   /* keep the next file line   */
                    new_count--;
                    read_line(1);
                    old_count--;
                    keep_line();
                } else if (sign == '+') {   /* keep the line from patch  */
                    new_count--;
                    keep_line();
                } else {
                    return !!puts("malformed patch");
                }
            }
        }

        if (file)
            finish_file();
        if (!got_line)
            return 0;

        set_filename();
        file = 0;
    }
}
