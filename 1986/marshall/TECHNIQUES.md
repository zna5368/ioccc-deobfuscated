# 1986/marshall

Difficulty: Easy

1. ASCII art layout

The whole source is drawn as a picture of a steam locomotive. Keywords,
identifiers and even single characters like `;` and `,` are scattered across
the page with large runs of spaces, so a single statement such as
`char **argc, r;` is split over two lines and mixed in with pieces of other
statements. The compiler ignores the spacing, so running the code through a
formatter (`clang-format`) puts it back on normal lines.

2. Shuffled parameter names

`main(r, argv, argc)` uses the familiar names in the wrong places. The first
parameter (normally `argc`, the argument count) is called `r`, and the third
one (normally the environment pointer) is called `argc` and declared as
`char **`. A reader who trusts the names gets the wrong idea about what each
variable holds. None of them are actually needed, so simple.c uses
`main(void)`.

3. Old K&R style declarations

`main(r, argv, argc) char **argc, r; char *argv[];` and `P(a) char a;` use the
pre-ANSI style where the parameter types are listed after the parentheses
instead of inside them. Combined with the art layout, these type lists are easy
to mistake for ordinary variable declarations.

4. Macro hidden inside the picture

`#define x  int i, j, cc[4];   printf("      choo choo\n");` sits in the middle
of the drawing, and the next line starts with `x`. That lone `x` is the macro
being used: it declares the variables and prints the only output of the
program. A beginner would easily read the `printf` as part of the `#define`
line and miss that it only runs because of the `x` on the next line.

5. Decoy code that does nothing

`extern int errno; char grrr;` declare variables that are never used. The
function `P` takes a `char`, evaluates it and returns nothing useful. The `if`
condition `P(!i) | cc[!j] & P(j) > 2 ? j : i` mixes uninitialized variables
with the meaningless results of `P`, and the body `*argv[i++ + !-i]; _exit(1);
printf("%d", P(""));` reads a value and throws it away, exits, and then has a
`printf` that can never run. All of it is noise meant to look important; in
practice the branch is not taken and `main` just returns.

6. Code hidden in comments

`/*for (i=0;;i++); */`, the argument `1/*argv[(int)*argc-2/cc[...]|-1<<4]*/`
and `/*while( a > " B " * - by EricMarshall- )*/` look like code but are
comments. They were the original infinite loop and odd exit value that broke
on modern compilers and were commented out (see marshall.alt.c). The last one
also hides the author's signature "by EricMarshall" inside the drawing.
