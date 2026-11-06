# 2019/diels-grabsch1

Difficulty: Hard

1. Meaningless single-letter names

Every variable is one letter: `a` is the code width state, `b` the bit
buffer, `c` the hash table, `d` the bit position, `e` the next free code,
`f` the current prefix code, `g` the input byte (and later the lookup key),
`h` the hash/probe step (and later the "not found" flag) and `i` the hash
slot. Several of them change meaning halfway through an iteration.

2. Magic number hidden in the table size

The table has `178819` entries, but only the first 69001 are ever used.
The odd size is there for the header: after the fill loop `a` equals
178819, and `a*=53` gives 9477407 = `0x909D1F`. The loop
`for(...;a;a>>=8)putchar(a)` prints the bytes `1F 9D 90`, which is the
compress(1) magic number followed by "block mode, 16 bits". The same loop
also leaves `a` at zero, ready to be reused as the width state.

3. Constants built from other constants

`i=d-9` sets `i` to -1 (since `d` is 8), which is the "empty slot" value
used to fill the table. `b=a` sets `b` to 0. `return!53` returns 0.

4. Code width packed into one encoded number

Instead of storing the width directly, `a` holds 0 to 7 for widths 9 to 16
(`a%8+9`), and the special value 15 means "16 bits and the table is full".
Because `15%8` is 7, the width formula still gives 16. The test `a<8*h`
checks both "the code was not found" (`h` is 1) and "the table is not full"
(`a` is below 8) in a single comparison.

5. Width-change rule squeezed into a nested ternary

`a=e<(512<<a%8|(a<7))||f>256?a:a>6?15:a+1` decides when codes get one bit
wider. `512<<a%8` is 2 to the power of the next width, and `|(a<7)` adds 1
to it below 16 bits, so the width grows only once `e` is past that power.
At 16 bits there is no `+1`, so reaching 65536 sets the "full" value 15.
`f>256` keeps the old width while a string is still being matched, so the
check only happens when a new string starts.

6. Packed hash table entries

Each slot stores the key (prefix code times 256 plus the byte) in the high
bits and the assigned code in the low 16 bits: `c[i]=g*65536L|e++`. Lookups
compare `c[i]>>16` with the key and read the code back with `c[i]%65536L`.
Empty slots are -1, so `c[i]>-1` means "slot in use".

7. Disguised modulo in the probe loop

`i+=i+h<69000?h+1:h-69000` looks strange, but it is the same as
`i=(i+h+1)%69001`: when adding `h+1` would go past the end, it wraps around
by subtracting the table size. This is double hashing, where the hash value
is also the step used to look for the next slot.

8. Reused variables and arithmetic tricks

`g` is first the input byte, then `g+=f<<8` turns it into the lookup key.
`h` is first the hash, then `h=c[i]<0` turns it into a 0/1 "not found"
flag. Later `f=h?g-f*256:...` gets the input byte back by subtracting the
prefix part from the key, instead of keeping the byte in its own variable.

9. Multiplying by a flag instead of using `if`

`b|=h*f<<d` and `d+=h*(a%8+9)` add the code to the bit buffer only when
`h` is 1. When the string was found, `h` is 0, so both lines add nothing
and the output loop runs zero times.

10. Bit buffer with an offset

The bit buffer `b` starts writing at bit 8 (`d=8`), and the output loop
`putchar(b=b>>8)` shifts first and then prints, so the low byte always holds
the byte just printed. The final loop `for(d+=a%8;d>-1;d-=8)` uses an
offset of `a%8` instead of the full width to work out how many bytes are
needed to flush the last bits.

11. Missing padding works by luck of the numbers

compress(1) pads its output when the code width changes, but this program
never writes any padding. It still works because each width is used for
exactly 2 to the power of (width - 1) codes, which is always a multiple of
8, so the bits already end on a whole group. The program also never sends
the CLEAR code 256, which is why `e` starts at 257.

12. Split identifier and dense layout

`putc\` and `har(a)` are joined by a backslash-newline into `putchar`, so
the line break falls in the middle of a name. The program is written as a
block of exactly the same size and shape as a paragraph in the README, with
no indentation, and `for(;cond;)` loops are used where `while` would be
clearer.
