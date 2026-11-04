# 2005/aidan

Difficulty: Hard

1. Variable names that look alike

Almost every name is a letter that looks like a digit or another letter: `I`,
`l`, `l0`, `O`, `o`, `S`, `C`, `c`, `L`, `i`. `O` is the depth of the guessing
stack, `l` is a cell number, `l0` is a counter, a digit or a success flag
depending on the line, and `L` is the program mode. The labels are `l0`, `lO`,
`l`, `O` and `O1`, so `goto O` and `S[O]` use the same letter for a label and a
variable. In simple.c they are called `depth`, `cell`, `mode` and so on, and
the labels are replaced by functions and loops.

2. Macros that rename an unfinished piece of code

`#define s S].s[l` and `#define S S[O` open brackets that the code using them
closes, so `s]` really means `S[O].s[l]`, the current cell of the frame on top
of the stack. `#define S S[O` is defined in the middle of `main`, so `S->O=0`
before it means `S[0].O` and `S++].I` after it means `S[O++].I`. The same
trick turns `S,82]` into `S[O,82]`, which the comma operator makes `S[82]`,
and `S&0]` into `S[O&0]`, which is `S[0]`.

3. The variable `i` changed into a random number by a macro

`i` is the seed of a random number generator. After the line
`#define i (int)(81.0*(i=...)/2147483648.0)` every use of `i` steps the
generator and gives a number from 0 to 80. So the plain statement `i;` in the
input loop changes the seed, `i%9+1` is a random digit and `i%81` is a random
cell. simple.c has a `next_random()` function for this.

4. One macro used for two different jobs

`O(o,k)` is used as `O(l/9,row)` to cross candidates off a row, and as
`O(l/9,)` with an empty argument to look for a digit that fits in only one
place of a row. The macro turns `k` into a string with `#k` and tests
`*#k`: `"row"` starts with a letter, `""` starts with the end-of-string byte,
so the same text compiles to either job. `Su(row,col,box)` calls it for rows,
columns and boxes, and `Ba(k,a)` prints the error message `!!! row 3` from
the same name. simple.c splits this into `eliminate()` and
`hidden_singles()`.

5. All code in main with goto

There are no functions apart from `main`. Solving, guessing, undoing a wrong
guess and generating puzzles all jump between labels inside one endless
`for(;;)` loop. For example a contradiction jumps to `l`, which goes back one
level of the stack, removes the wrong digit and starts again at the top of
the loop.

6. A switch with a case inside an if

The `switch(L)` at label `O` has `case 3:` placed inside the body of the
`if` that belongs to `case 2:`, and the `O1:` label is placed inside a `for`
loop so that `goto O1` jumps into the loop without running its start code.
This lets the generator start a new scan, keep scanning or put a digit back,
all from the same few lines. In simple.c it is the `remove_digits()`
function with an ordinary loop.

7. Bits for the cell contents

Each cell is one `int`. Bits 1 to 9 say which digits are still possible,
bits 10 and up hold the digit placed in the cell, and bit 0 marks a cell the
generator already tried to empty. `01776` (octal for `0x3fe`) means "every
digit is possible" and `1023` means "every digit is possible and tried". The
loop `for(I=0;~o&1&&(o/=2);I++);` finds the lowest set bit, and `o-1||...`
checks that only one bit was set. simple.c names these with `VALUE()`,
`GIVEN()` and `ALL_CANDIDATES`.

8. Encoded characters and strings

There are no character constants: `getchar()^48` turns `'0'`..`'9'` into 0..9,
`30` is `'.'^48`, `46` is `'.'` and `I|48` turns a digit back into its
character. The only character constant, `'Q'`, is the number 81. The row
separator `"\007      &       &      \n"` is decoded while the first 22 cells
are read by XORing each byte with 13, which gives
`"\n------+-------+------\n"`.

9. Logic operators and commas used as if statements

Statements such as `O||printf(...)`, `O>w&&(w=O)` and `l[c]<0||...||(...,C++)`
only run their right side when the left side allows it. The condition
`if(s]>>10||(s]&=c[o]),!s])` does a mask with the comma operator and then
tests the result. `l[c]` is `c[l]` written backwards.

10. One number used for several things

The struct fields `I`, `l` and `O` are the guessed cell, the guessed digit
and the first digit tried for normal frames, but in frame 82 the generator
uses them for the removed cell, the start of the scan and the removed digit.
`L` is read from the first character of the argument: `U` gives 6, any other
argument gives 1, and later `L|=2` turns the "generate" mode 1 into 3 to start
removing digits.
