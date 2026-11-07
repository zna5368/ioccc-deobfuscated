# 2025/mattpep

Difficulty: Terrifying

1. Look-alike identifiers (homoglyphs)

Many variables have names that look like ordinary Latin letters but are really
Greek or Cyrillic letters, or full-width punctuation. `X` and `x` are Latin, but
`Х` (Cyrillic Ha) is the output pointer. `E` is the encode flag, `Ε` (Greek
Epsilon) means "write output now", and `Е` (Cyrillic Ie) means "read input now".
`A` is a Latin `char **`, while `А` (Cyrillic) and `Α` (Greek) are two different
strings. `О` (Cyrillic O) is a function pointer and `Ο` (Greek Omicron) is the
column counter. `N`/`Ν`, `p`/`р` and `B`/`В` are also pairs of different
variables. Even `｛` and `｝` are not braces: they are full-width characters used
as variable names (a "first block" flag and the character being converted). The
compiler sees different names, but a person reading the code sees the same
letter twice.

2. Punctuation hidden behind macros

`#define B ))`, `#define Β ])` (Greek Beta), `#define C ;` and `#define D EOF`
hide closing brackets, semicolons and `EOF` behind single letters. Because of
this you cannot match up parentheses by eye, and `getchar( B` is really
`getchar())`.

3. Misleading macros

`#define R <<` and `#define L >>` make it look like `L` means "left" and `R`
means "right", but they shift the other way. `#define б4 64+64` looks like the
number 64 (`б` looks like `6`), but it is `64+64` with no parentheses. So
`(б4+б4)/б4/64` becomes `(64+64+64+64)/64+64/64`, which is 5, and
`64*64/(б4+б4)-64/64` is 15.

4. Constants pulled out of strings

Almost every number is computed by looking up characters in the two strings
`А` and `Α`, sometimes with several layers of indexing. For example
`А[А[7[А]R 1]+1]L 1` is `А[А[18]+1] >> 1`, which is `'V' >> 1`, which is `'+'`.
`Α['$'L(*Α L('\f'L 1))]/Α['\r'-1]` works out to `'<' / '\f'`, which is 5.
The strings look like C code (`int write_0utput() { putchar(c); }`,
`int x=2053;`) to make you think they are comments or dead text, but they
are really lookup tables of numbers. To read them you have to count character
positions by hand.

5. Split and disguised strings

String literals next to each other are joined by the compiler, so the program
splits them into pieces such as `"int y=3502; ""\f\rt\ttA<=!"` followed by
`"azAZ09/+"`. The comment `/* The base64 character set */` makes `azAZ09/+`
look like a real alphabet table, but only single characters of it are ever used
as numbers. The `*/` inside the first string also makes it look like a comment
ends there.

6. State machine driven by recursive main()

There are no loops for the actual work. A global variable `a` picks what
`main` does when it is called: 1 handles the command line, 2 converts a single
character between its base64 value and its letter, and 5 either reads input or
writes output, depending on the `Е`/`Ε` flags. Each byte written and each
character read while decoding is one more recursive call to `main(5, A)`, and
converting a character means setting `｝`, setting `a=2`, calling `main` and then
reading back the result from `x`. In simple.c this is replaced with ordinary
`while` and `for` loops and plain helper functions.

7. Swapped function names

The function called `decode()` actually encodes three bytes into four base64
characters, and `encode()` actually decodes four characters into bytes. The
function pointer `О` decides which one gets called, so the names are never
checked against what they do.

8. Short-circuit operators instead of if statements

Expressions like `(р>1)&&(p[2]=...)`, `(O[2]!='=')&&four++` and
`X&&(x=...)` use `&&` as an `if`: the right side only runs when the left side is
true. Long chains of `?:` replace `if/else if` ladders when mapping a value to
its base64 letter and back.

9. Comma operator to pack statements

Lines such as `p[0]=(X=0,((｝=(O[0]),a=2,main(X,A),x))<<2)` do several things in
one expression: set a flag, set the input character, pick a state, call `main`,
and finally use `x` as the value. The comma operator runs each part from left
to right and keeps only the last value.

10. Mode chosen by an odd/even character

`E=(i-1)&&j[1][0]&1` encodes when the first character of the first argument has
an odd ASCII code. This is why `encode`, `3052`, `Would you...` and
`#$@!ing ...` all encode, while `decode`, `2503`, `fucking ...` and no
argument decode (and also why `--decode` encodes). The program never compares
the argument with a word.

11. The value four is not a constant

The variable named `four` is a counter. It counts the characters left to read
in a block, then the number of bytes left to write, and can be 1, 2, 3 or 4
(or even -1 for a moment). In simple.c it is split into `remaining` and
`out_count`.

12. Dead code and unreachable branches

`if (a == -А['*'])` compares an `unsigned char` with -32, and the next block
compares it with -11, so neither block can ever run, even though they contain
`getchar` and `return 0` code that looks important. `!Е && !Ε` and
`Е && Ε` can never be true because only one of those flags is set at a time,
so `main(a=4,A)` and `main(a=3,A)` are never called. The character `':'`
decodes to 0, which valid input never uses. `encode()` begins by setting
`p[0]`, `p[1]` and `p[2]` to 1 just before overwriting them. `main(А['2'],A)`
passes 59 as `argc`, which is ignored. All of these exist only to waste the
reader's time.

13. Lying and misleading comments

`/* The Atomic number for Gadolinium is 64*/` sits above the `б4` macro. (It is
true, but `б4` is not 64.) `/* Output routine */` labels variable declarations,
not a routine. The comment tokens inside the strings look like real comments.

14. Undefined shift used as zero

The column counter starts as `Ο = А[15] >> Α['\b']`, which is `'t' >> 48`.
Shifting an `int` by more than 31 bits is undefined behavior in C. On x86 it
happens to give 0, so the counter starts at 0. simple.c simply writes
`column = 0`.

15. Misleading layout and code blobs

The bodies of `encode()` and `decode()` are dense rectangles with expressions
broken across lines in the middle of tokens like `O[1` / `]`. At the end of
`main`, the indentation suggests that `if (four+E!=E)`, `if (!Е && !Ε)` and
`if (Е)` are top-level statements. In fact the braces put the first one inside
`if(Ε)` and the others inside `if(a==5)`, so you have to count every brace by
hand to see the real nesting.
