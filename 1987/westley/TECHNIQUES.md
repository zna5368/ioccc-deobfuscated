# 1987/westley

Difficulty: Easy

1. Mirrored identifier names

Every name is paired with its reverse so the code reads the same in both
directions: `rahc`/`char`, `niam`/`main`, `tni`/`int`, `rof`/`for`,
`rahctup`/`putchar`, `deliver`/`reviled` and `LACEDx0`/`0xDECAL`.
`redivider` is itself a palindrome. The names look like keywords written
backwards, so the eye keeps reading them as keywords. In simple.c they become
`palindrome`, `newline`, `p` and `length`.

2. Symmetric "ink blot" layout

Each line of the source is centered so the file is left/right symmetric, and
single tokens such as `=`, `,` and `[ ]` sit on lines of their own. Even the
comments `/*\}` ... `\*/` and `/*\` ... `{\*/}` are mirror images, and they
hide a `{` and a `}` inside the comments so the braces appear balanced on the
page. This layout hides where one statement ends and the next begins.

3. Misleading declarator (`*` binds to only one name)

`*deliver,reviled = 1+1` looks like it declares two pointers, but in C the
`*` only belongs to `deliver`. `reviled` is a plain `char` holding the
number 2. Later `++reviled`, `reviled--` and `reviled--` move it to 3, 2 and
finally 1, so `reviled * *deliver` is simply "the current character" (times
one) and the loop condition is just "not at the end of the string".

4. Dead and do-nothing code

The first loop `for(;(int)(tni);)` never runs because `tni` starts at 0.
`niam` and the extra `/` in `"\n/"` are never used. `rahctup * putchar(...)`
multiplies a value and throws the result away, and the variable `rahctup`
only stores results that are never read. They are there only to fill out the
symmetry.

5. Constant that cancels itself out

`LACEDx0 = 0xDECAL` and later `(tni) = (int) - 0xDECAL + LACEDx0 - rof`.
`0xDECAL` is the hex number `0xDECA` with an `L` (long) suffix, so the two
`0xDECAL` terms cancel and the expression is just `-rof`. `rof` was set to
`-1 - tni` on the last pass of the counting loop, so this puts the string
length plus one back into `tni`.

6. Casts and parentheses that do nothing

`(int) (tni)`, `(int)--(tni)` and `(char) * (rahc)` add casts to values that
are already the right type. They pad lines to keep the shape symmetric and
make simple expressions look complicated.

7. Comma operators and off-by-one counting

The loops pack several updates into one place, such as
`for ((tni)++,++reviled;...;deliver++,++(tni))` and
`for (reviled--,(int)--(tni);...;(int)--(tni),--deliver)`. The counter `tni`
starts at 1, is one too large after counting, and is decremented once before
the printing loop, so the number of characters printed only works out after
following every `++` and `--`. In simple.c one loop measures the length and a
second loop walks backwards printing each character.

8. Local declaration of a library function

`int ..., putchar()` declares `putchar` as a function inside `main` next to
ordinary variables. That is why the judges note that `putchar()` must be a
real function and not only a macro.
