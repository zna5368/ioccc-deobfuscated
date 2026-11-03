# 1991/cdupont

Difficulty: Medium

1. Source code used as data

The program opens its own source file with `fopen(s,t)`, where the Makefile
defines `s` as `__FILE__` (the name of the file being compiled) and `t` as
`"r"`. The first 26 lines are read into `u`, and the message letters are picked
out of that text. Every comment, label and space in the file is placed so the
right letter lands at the right row and column. That is why reformatting or
pre-processing the file (or removing `sorryfor_this_unused_but_very_needed_label`)
breaks the output: it moves the letters.

2. Letters chosen by a chain of coordinates

`x=u[gi][m]` is the letter to print and `f=u[m][gi]` (the mirrored cell) is a
hidden key. The next position is computed from both with
`m=(i+1)*(4*x+3*f)%032` and `gi=(i+1)*(x+2*f)%0x1a`. You cannot see the
message without following every step, and cells on the diagonal (`m==gi`) are
replaced by the fixed pair `'p'`/`'h'`. The loop stops when the letter `.`
(`0x2e`) is printed.

3. Overlapping array rows

`u` is declared as `char u[0x50][032]` (rows of 26 bytes) but `fgets` reads up
to `0120` (80) bytes into each row. Long lines spill into the following rows
and are only partly overwritten by the next line, so some grid cells hold text
from an earlier line rather than the line you would expect.

4. Word lengths packed into a number

The spaces between words come from `cd=0x5765248d`. Written in octal it is
`12731222215`, and each octal digit is the length of one word. The code at label
`hxi` pulls one 3-bit digit out per word with `cd&(7<<3*(014-++ri))`, shifts it
down and clears it from `cd`. The counter `httxkbl` counts letters until it
matches the length and a space is printed.

5. Misleading operator precedence

`n=cd&0x40000000L>>0x1e` looks like it masks a high bit, but `>>` is applied
before `&`, so it is really `cd & 1`, which gives the length (1) of the first
word.

6. Spaghetti goto control flow

There are no real loops besides a `for` whose body jumps out with
`goto daswjhkls` and back in with `goto vhjsgfdy1l1gjhd`, a label placed
inside the loop body. Every other part of the program is a chain of `goto`s
between scattered labels, so the order the code runs in is very different from
the order it is written in.

7. Look-alike labels and meaningless names

Labels such as `c0g0`, `c0gO`, `cOgO` and `cOg0` differ only by zero versus
letter O, and `vhjsgfdy1l1gjhd` versus `vhjsgfdyl1lgjhd` differ only by the
order of `1` and `l`. Variables like `httxkbl`, `gi` and `cd` have no meaning,
and the comments are filler whose letters exist only to be read as data.

8. Mixed number bases

The same values are written in different bases to hide them: 26 appears as
`0x1a` and `032`, 12 as `014`, 10 as `012`, 80 as `0x50` and `0120`, and the
characters `.` and `p`/`h` as `0x2e`, `0x70` and `0x68`.

9. Macro hiding putchar

`#define yank(x) putchar(x)` is defined in the middle of an `if` statement so
the space and newline output looks like a call to an unknown function. In the
original version (`cdupont.orig.c`) the macro even contained the opening quote
of a character literal, `#define yank putchar('`, which only works because the
preprocessor pastes the pieces back together.
