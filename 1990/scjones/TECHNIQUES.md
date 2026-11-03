# 1990/scjones

Difficulty: Hard

1. Trigraphs

Every `#`, `{`, `}`, `[`, `]` and `\` in the source is written as a
three-character trigraph: `??=`, `??<`, `??>`, `??(`, `??)` and `??/`. For
example `char*a??(??)=??<` is really `char*a[]={` and `??=include<stdio.h>` is
really `#include<stdio.h>`. The compiler swaps trigraphs for the real
characters before it does anything else, even inside strings and character
constants, so `'??/n'` is the newline `'\n'` and `??/"` inside a string is an
escaped quote `\"`. Most people have never seen trigraphs, so the code looks
like line noise.

2. Avoiding a trigraph with `?\?`

The program has to print the text `??=` without the compiler turning it into
`#`. It writes `"???/?="`: the first `?` is left alone, `??/` becomes `\`,
and the string ends up as `"?\?="`. The other strings work the same way, for
example `"???/?<"`. The escape `\?` is just a question mark, so the output is
`??=`. The trigraph and escape rules stack on top of each other, which makes
these strings very hard to read.

3. Self-reproducing program (quine)

The array `a` holds every line of the code that follows it, written as
strings. `main` first prints `char*a??(??)=??<`, then each line again as a
quoted string, then `0??>;`, and finally each line again as plain code. The
output is exactly `scjones.c`. Because the data and the code are copies of
each other, the program appears twice and you have to read both copies to see
that they match.

4. Macros that hide keywords

`#define b break;case` lets each switch case run straight into the next label:
`s("???/?=")b'??<':` really means `fputs("??=",stdout); break; case '{':`.
`s(p)` and `c(p)` hide `fputs` and `fputc`, and `o` hides `stdout`. Each of
these macros ends with its own semicolon, so the code looks like it is
missing statement terminators.

5. Short-circuit `&&` used as an `if`

`f&&c('"')` and `f&&s("???/?/")` use `&&` to run the right side only when the
flag `f` is non-zero, which hides an `if (f)` statement. Case `0` (the end of
the string) uses `f&&s("\",")` to close the quoted string only when printing
the array.

6. `else` jumping into a `case` label

`if(f)s("???/?/n") else case'"':if(f)s(...) else default:c(*p)` puts the
`case '"':` and `default:` labels inside the `else` branches of `if`
statements. When `f` is zero, the newline case falls into the quote case,
which falls into the default case and prints the character unchanged. When
`f` is set, each case prints its escape sequence and leaves the switch. This
nests the switch cases inside `if` statements in a way most programmers would
not expect.

7. Old-style K&R function definitions and implicit `int`

`void t(p,f)char*p;` declares the parameter types after the parameter list,
and `f` has no type at all, so it defaults to `int`. `main()` has no return
type either. `t` has no braces of its own around its body. Its `{` is
`??<`, and the three closing braces are hidden in `??>??>??>`.

8. Uneven line breaks

The code is split at odd places to keep every line under 72 characters. For
example `for(p=a;*p` / `;p++)` and `exit(!ferror(o)&&` / `!fclose(o)?...`
cut statements in the middle, so the structure of `main` is hard to see.
