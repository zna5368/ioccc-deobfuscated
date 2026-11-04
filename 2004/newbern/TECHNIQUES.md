# 2004/newbern

Difficulty: Hard

1. Code shaped like a font glyph

The source is laid out in the shape of the letters from `newbern.dat`, with
large runs of tabs in the middle of statements. Tokens like `t(O`, `*)` and
`*o;` are split over lines so they fit the outline, and string literals are
cut into tiny pieces such as `"B" "0" " " "B" "" "B"`. The compiler joins
neighbouring string literals together, so these pieces (and the empty `""`
strings) become one long string.

2. Macros that hide keywords

`G` is `return`, `Z` is `while`, `y` is `fclose`, `end` is `static`,
`e(s)` is `exit(s);` and `t(x)` is `typedef x`. Reading the code means
replacing every one of these letters with its real meaning in your head.

3. Macros that duplicate their argument

`b(x,o)` expands to `x o x` and `Y(x)` expands to `x;x`. So `b(*,p=) s` becomes
`*p = *s`, `b(i, ),w` declares `i i, w`, `b(!, i &&)(*p >= n)` becomes
`!i && !(*p >= n)` and `q=Y(*(o)x)=Y(u)=Y(x)=q` is really the four statements
of a linked list reversal. `b(c,;)` inside `static U b(c,;) ?R(c):(c='\n');`
both declares `c` and starts the next statement with it.

4. Single letter names that reuse each other

Types are renamed to `U` (`char`), `H` (`char *`), `i` (`int`), `O` (the
struct) and `o` (`O **`). The name `i` is both the `int` type and a variable
of that type (`i i;`), `l` is both a function and `argv`, and `X` is the font
table. A local variable can hide a type with the same name inside one block.

5. Struct fields accessed through casts

The linked list node is `struct O { O* l, **h; void* L; }`. Instead of
writing `x->l`, the code writes `*(o)x`, which casts the node pointer to a
pointer to pointer and reads the first field. The field `h` is never used.

6. Program modes chosen by argument count

`main` is one big `switch (argc)`. 1 argument is the Morse encoder, 2 is the
glyph encoder, 3 prints a banner and 4 prints an "encrypted" banner. The cases
fall through into each other on purpose: case 3 loads the font and runs into
case 4, which runs into `default`, which runs into `case 0` (the glyph
printer).

7. goto into the middle of other blocks

`goto u` jumps from the Morse reader into the glyph printer, `goto D` jumps
back, `goto j` jumps from case 4 back up into the font loader of case 3 and
`goto i` loops the list reversal. `goto n` jumps backwards into the body of a
`while` loop. The trailing `while (Q++ < 1)` loop at the end of `main` can never
run and only exists to hold more confusing code.

8. Recursive main

After each banner character, `main(4, l)` is called again to print the next
one, using `argv[2]++` to step through the text. Because case 4 has a `static`
flag, the second call knows whether the font is already loaded and whether to
"encrypt".

9. Computed constants

The base value `'+'` used to decode pixel rows is calculated by a strange loop
with `goto n`, `n % 7` and a comparison against `'&'`, which ends with
`n == 43`. Other numbers are also disguised: `6<<6` is 384, `0400` is 256,
`j<<8` is 256, `(i-2+9)%(sizeof(s))` steps by 7 modulo 256, `001<<(w%6)`
is a bit mask and `0xa` and `32` are `'\n'` and `' '`.

10. Six pixels packed into one character

Glyph rows are stored as printable characters. Subtracting `'+'` gives a
number from 0 to 63 whose six bits are six pixels, lowest bit first, printed
as `'#'` (`32 + 3`) or `' '` (`32 + 0`) by `R(32+(i&1)*3)`. The built-in Morse
table in `I[]` uses the same encoding, so `"aH"` means the letter `a` is the
row `# ###` (dot dash). Each word after the key character goes to the next
character, so one string holds the whole alphabet.

11. Function pointer and array index tricks

`R` is a pointer to `putchar`, so every `R(...)` prints a character. Array
indexing is written backwards, as in `i[e]`, `i[X]`, `1[l][0]`, `2[l]` and
`1[x]`, which works because `a[b]` is the same as `b[a]` in C.

12. Hidden list reversal and rotation

Rows are pushed onto the front of each glyph list, so they end up upside
down. The loop at label `i` walks the table in steps of 7 (which visits all
256 entries) reversing every list. For the "encryption", case 4 shifts the
whole table down by one entry each time it is called, so every following
letter is drawn using the glyph of the next character after it.

13. Side effects in odd places

`k()` prints a newline only after the first time it is called because its
static `c` starts at zero. `(*s)&&h(X,s+j,*s)` uses `&&` as an `if`, and the
macro `m(x)` hides a `malloc` inside a `do { } while (0)` block.
