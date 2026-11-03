# 1995/spinellis

Difficulty: Medium

1. Data hidden in whitespace inside a comment

The big `/* ... */` comment looks like random spaces, tabs, newlines and
stray `{`, `}` and `;` characters, but it is actually the encoded message.
The compiler ignores comments, so the data costs nothing as code. The
program never reads its own file directly; you must feed the source in with
`./spinellis < spinellis.c`, and it then decodes the comment it was given.

2. Exploiting the contest size rule

The rules counted characters without whitespace, and without `;`, `{` or `}`
when they are followed by whitespace. The comment is built only from exactly
those characters, so almost the whole file is "free". The `*` that opens the
comment and the leftover `comments!=style` at the end of it are the only
parts of the comment that count.

3. Character position used as a number

`b()` reads a character and computes `strchr(s, getchar()) - s`, which is
the position of that character in the string `"{ }\t;\n*"`. So `{` means 0,
a space means 1, `}` means 2, a tab 3, `;` 4, a newline 5 and `*` 6. The
first loop `while(b()!=6);` skips everything up to the `*` that starts the
comment.

4. Mixed number bases

`n()` builds a code from one base-6 digit (`b()*3`) and two base-3 digits
(`b()/2`, which pairs the six symbols into three values). The result is
`9*d6 + 3*t1 + t2`, a number from 0 to 53.

5. Nested ternary operator as a lookup table

The long `c==0?' ':c==1?'\n':...` chain in `n()` turns the code into a
character: 0 is a space, 1 a newline, 2 a tab, 3 to 32 are `a` to `~`, and
codes 33 and up pick a group of three characters (`(c-32)*3+' '`) where one
more base-3 digit picks which of the three. A decoded `$` ends the program.

6. Short, meaningless names and a hidden tab

The functions are just `b` and `n`, the global `c` is shadowed by a local
`c` in `n()`, and both the string `s` and the ternary chain contain a real
tab character between quotes, which looks like ordinary spaces when read.

7. Misleading comments

The README boasts that 96% of the lines are "comment text rendering the
program readable", and that the comments pass the spell checker. The comment
contains no words at all, which is the joke: it is the data, not
documentation.
