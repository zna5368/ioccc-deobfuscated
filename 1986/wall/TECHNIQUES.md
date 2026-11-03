# 1986/wall

Difficulty: Hard

1. Macros that rewrite the meaning of `main`

`#define main(o,oo)oo(o){` swaps its two arguments and adds an opening brace.
So `main(;;,for)` becomes `for(;;){`, `main(c,C_(s))` becomes `switch(c){` and
`}main(,cc)` becomes `}cc(){`, the start of a new function. The word `main`
appears many times but only the very last one, after `#undef main`, is the
real `main()` function.

2. Keywords built from pieces with token pasting tricks

`#define C_(sand)_O(sand)witch` together with `#define _O(s)s` turns `C_(s)`
into `s` followed by `witch`, giving the keyword `switch`. In the same way
`O_(sy)` becomes `system(ccc(`. You will not find the words `switch` or
`system` anywhere in the source code.

3. Punctuation hidden in macros

Statements are glued together by macros that hide C punctuation: `_` is
`;case`, `__` is `;break;`, `_C` is `;return`, `C` is `char*` and `c_(x)`
is `c x=`, so `c_(+)025` means `c += 025`. The code reads like random
underscores while it is really a `switch` with many `case` labels.

4. A macro that is redefined in the middle of the code

`keyboard` is first defined as a string of strange bytes and is used to
initialise the global `o`. Later, inside the function `ccc`, it is redefined
as `"dijs QH.soav ..."` and the second use of `keyboard` gets this new string.
The same name means two different things depending on where it appears. The
`#ifndef lint` around it hides the trick from the `lint` checker.

5. Encoded strings and a hidden character translation table

The commands given to `system` are written in a scrambled form and decoded by
`ccc` at run time. `"dijs QH.soav Vdtnsaoh DmfpaksoQz;kkt oa, -dijs"`
becomes `echo "Dvorak Keyboard Emulator";stty raw -echo` and
`";kkt -oa, dijszdijs QQ"` becomes `stty -raw echo;echo ""`. The scrambling
is exactly the QWERTY to Dvorak key mapping, so the encoded text is just what
you would type on a QWERTY keyboard to get the real text on a Dvorak one.

6. Lookup values packed into numbers

The translation for punctuation characters is written as `_c(8098)`,
`_c(6055)`, ... Each number holds two values: the low 7 bits
(`8098 & ('|'+3)` which is `8098 & 127`) are the input character and the
upper bits (`8098 >> ('\n'-3)` which is `8098 >> 7`) are the output
character. For letters, a table of offsets in the first `keyboard` string is
added to the character and then several `case` labels written in octal
(`0214`, `0216`, `0207`, `0233`) fall through into each other to adjust it.
In `simple.c` all of this is replaced by two plain strings showing the
QWERTY and Dvorak layouts side by side.

7. Constants disguised as expressions

Simple numbers are hidden behind character arithmetic and other tricks:
`'|'+3` is 127, `'\n'-3` is 7, `'z'+5` and `'~'+1` are both 127, `'-'-1` is
44 and `' '-1` is 31. `__LINE__-007` uses the current line number (38) to
get 31, so moving that line would break the program. The `switch` on
`0xb+(c>>5)` uses `'\v'` (11) and `'\f'` (12) as case labels to choose
between control characters and punctuation.

8. Pointer arithmetic into a data string

`O o+' ',3)` becomes `write(1, o + 32, 3)`. Position 32 of the first
`keyboard` string holds `"\r\n"` followed by the string terminator, so this
prints a carriage return and newline when Return is pressed.

9. Confusing names

Every name is a variation of the letters `c` and `o`: `c`, `cc`, `ccc`,
`cccc`, `o`, `O`, `O_`, `_O`, `C`, `C_`, `_C`, `c_` and `_c`. Variables,
functions, macros and types all look alike so the reader cannot tell them
apart.

10. Joke formatting that hides the structure

Statements are split over many short lines, with case labels such as `0214`
and `:` on separate lines, so the code has no visible structure. The
"documentation" is the compiler warning about `keyboard` being redefined,
which tells you the program is a keyboard emulator.

11. Results that are never used

`write(1, ccc(c), '\0')` translates the typed character but writes zero bytes
of the result. Because `ccc` translates a copy made by `strdup`, the typed
character itself is not changed either, so this line has no visible effect
in this version of the program.
