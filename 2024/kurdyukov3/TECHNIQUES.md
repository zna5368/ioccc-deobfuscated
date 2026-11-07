# 2024/kurdyukov3

Difficulty: Hard

1. Macros that generate whole switch statements

`C(a,b)` expands to `case a:b; break;`, `X(n,t,o)` builds one arithmetic case
out of `C`, `Y` makes four `X` cases and `O(n,t)` makes sixteen `Y`/`X` cases.
So the six short words `O(32,unsigned)O(48,int)...O(112,signed char)` turn into
96 `case` labels doing `+ - | ^ & * / % << >> == != < <= > >=` on six
different integer types. The real `switch` is only visible after the
preprocessor has run.

2. One macro body used twice for different operands

`R` is the code that decodes an operand (immediate, absolute address,
register, or register plus offset) and then shifts the control word with
`a>>=7`. It is written once and used twice in a row (`R c=b; R d=m[d+1];`), so
the two different operand fields are read by what looks like the same line.
Because of the hidden `a>>=7`, the later `switch(a>>9)` and `a&127` look at
different bits than they appear to.

3. Instruction fields packed into one word

Each instruction's first word holds three 7-bit addressing modes, a 2-bit
store size and the opcode. The code pulls these out with shifts and masks such
as `a&127`, `a>>4&24` and `a>>9` instead of named fields, so you have to work
out the bit layout by hand.

4. Registers hidden in memory

The program counter is `*m` (memory word 0) and `m[1]` is used for the loaded
length and for keyboard and mouse event data. There are no variables called
"pc" or "register"; jumps happen by writing to `*m`, either with
`*m+=b?d:0` or by an instruction storing into address 0.

5. Short, reused, meaningless variable names

Every variable is one letter. `c` is `argc`, then the argument counter while
loading, then operand 1, then the X event type, then a bit shift. `b` is the
byte counter while loading, then operand 2, then the result. `n` is a string
index, an addressing mode and a bit mask. `f` is the program file and later a
throwaway pointer for the frame buffer. `h` is the file table, but inside one
case it is also a local `XSizeHints h`.

6. Tricky bit tricks instead of plain comparisons

The window size check `~(-c&-b)>>12` is true only when both width and height
are between 1 and 4096. The file name filter
`b&&b-48>9&&(b|32)-97>25&&b-46?95:b` uses unsigned wrap-around so that
`b-48>9` means "not a digit" and `(b|32)-97>25` means "not a letter".
The unaligned read uses `m[b+1]*2<<~n%32` which is the same as shifting left
by `32-n` but avoids the undefined shift by 32.

7. Masked stores written with XOR

The `W(c)` macro stores only some bits of a word with
`m[d]=(m[d]|n c)^(~b&n)c`. It first sets all the bits under the mask and then
uses XOR to clear the ones that should be zero. It is applied twice with
`<<c` and `>>(32-c)` so a value can be split across two memory words.

8. Side effects packed into conditions

The loading loop condition
`(a=fgetc(f))<256||(a=x[c][n++]&255)||(n=0,--c)` reads the file, then the
command line arguments one byte at a time, and moves to the previous argument
using the comma operator, all inside the `for` test. `case 5` uses
`H=H?fclose(H),f=0:fopen(...)` to close or open a file in one expression, and
`while(b=2,I&&XPending(D))` resets the result on every loop pass.

9. Macro arguments that are not what they seem

`B(i,a,j)` starts an `if` and leaves the statement unfinished with a
trailing `b`, so `B(ButtonPress||b,xbutton,button)+=4;` becomes
`if(c==ButtonPress||b){ m[1]=...; b+=4;`. The `M(x)` macro pastes tokens with
`##` to build `KeyPressMask|KeyReleaseMask|` from just `M(Key)`.

10. Strings split across lines

Names like `stderr`, `timeval`, `PMinSize`, `XDefaultGC`, `PointerMotionMask`
and `XSync` are broken in the middle with a backslash newline, so searching
the source for them fails.
