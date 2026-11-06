# 2018/ferguson

Difficulty: Medium

1. Keywords hidden in compiler options

The Makefile passes `-DQ='typedef'`, `-D'g(o)'='goto o;'` and `-D'w(x)'="x:"`,
so the source never spells these out. `Q unsigned d;` is really
`typedef unsigned d;`, `g(f)` is `goto f;` and `w(case 'q')` is
`case 'q':`. You can't read the code without also reading the Makefile.

2. Single letter type names

`Q unsigned d; Q char E; Q int k; Q long u; Q size_t t; Q const char U;` give
one-letter names to ordinary types, so `k main(k a, E **V)` is really
`int main(int argc, char **argv)` and `t i,j` is `size_t i, j`.

3. Macros that do nothing

`H(main)` is defined on the command line as `r(main)`, and the source defines
`#define r(main) main`. So `H(x)` and `r(x)` both expand to just `x`. Nearly all
of `main()` is wrapped in nested `H( ... )` calls, which look like recursion or
a "special C parser" but disappear completely. The parameter is named `main` to
make it look like it has something to do with the `main()` function.

4. A declaration split by a fake prototype

`t i,j,` ends one line, then a comment says "prototype for special C parser
H() below" followed by `F H();`. `H()` expands to nothing, so this is really
`size_t i, j, F;`, declaring a variable `F`, not a function prototype.

5. Macros for common library functions

`L` is `toupper`, `Y` is `strlen`, `Z` is `printf`, `K` is `static`, `z` is
`do{` and `l` is `S-1`. Single letters hide which library calls are made.

6. Digraphs

The two-character spellings `<%` `%>` `<:` `:>` mean `{` `}` `[` `]`. They are
mixed with the normal symbols, for example `V<:s-1]` and `V[s-1:>`
both mean `V[s-1]`, and a `{` opened by `<%` may be closed by `}`. The keyboard
string `" !\"#$<%:>&..."` contains `<%:>` too. Digraphs are not replaced inside
strings, so a careless search and replace would change the keyboard.

7. Encoded strings

The default target `METHINKS IT IS LIKE A WEASEL` is written as the ASCII
codes `{77,69,84,72,...}`. The Easter egg target (the primorial 97#,
`2305567963945518424753102147331756070`) is stored as
`"stqvvwxzwtzuvvryusuxvtrqsruxttrxvwqxq"`, and each letter has `'A'` (65)
subtracted from it, so `'s'` (115) becomes `'2'` (50).

8. Hidden Easter egg condition

`n=N==97` sets a flag that is only true when the program is built with
`OFFSPRING=97`. Later, if no target was given, `T[j]=W[j]-'A'` replaces the
target with the decoded number. Also, `N` below 4 is quietly changed to 3, and
`N<4` forces "monkey" mode.

9. Control flow made of gotos and labels

The argument parser uses `goto o` to jump past the option code into the
target-copying code, `goto f` jumps back before `while (*++v)` instead of using
`break`, `goto h` skips to an empty label at the end of the loop body instead of
using `continue`, and `goto p` skips the whole sorting step in monkey mode.

10. Reusing names

`x` is a struct tag, a typedef and a local variable (`x x;`). `G` is both the
global generation counter and the struct member holding the genes. `F` is a
struct member and a local variable. `t` is a type name and also a parameter of
`X(k f,k t)`. `h` is both an `int` (end of options) and a label. `s` is the
argument index and later the "sorted" flag. `m` means "bad mutation rate"
while parsing and then "monkey mode" afterwards.

11. Side effects hidden in conditions

`for (s = 1; s < a && *V[s++]; )` moves to the next argument inside the loop
test, so the loop body uses `V[s-1]`. `if (((D=(F==Y(T)))||G==1)&&...)` sets the
"done" flag inside an `if`, and `if((m=(m||J||N<4)))` changes `m` inside the
test that prints "monkey at typewriter".

12. A swap that is not a swap

The sorting loop does `x=O[i+1]; O[i]=O[i+1]; O[i+1]=x;`. This looks like
swapping two elements, but it copies `O[i+1]` over `O[i]` and then puts the
same value back into `O[i+1]`. The fitter offspring is just copied into the
earlier slot.

13. Misleading comments

Comments call `#define r(main) main` a "Special C main() handler
(encapsulation)" and call `H()` a "special C parser", even though both do
nothing. The comments `/*NO*/ /*BLOB*/ /*CODING*/ /*THIS*/ /*IS*/ /*NOT*/ /*A*/
/*COMPACT*/ /*BLOB*/ /*OF*/ /*OCTETS*/` scattered through the code spell out a
joke and have nothing to do with the code next to them.

14. Blob formatting

The statements of `main()` are packed into a solid rectangle of text with lines
broken in the middle of expressions (for example `if (++` on one line and
`G==M)` on the next). This hides where statements and blocks begin and end.
