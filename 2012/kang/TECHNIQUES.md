# 2012/kang

Difficulty: Hard

1. Words encoded as octal numbers

`"ynwtsflrabg"[n%=11]-e?b:b*8+n` turns every word into a number. Only the
letters `y n w t s f l r` count, and each one is an octal digit (0 to 7, its
position in the string). Every letter found is "appended" to `b` with
`b*8+n`, the same way you append a digit to a decimal number with `x*10+d`.
All other letters (vowels, `h`, `d`, ...) are skipped, so `seven` becomes the
octal number `041` (`s`,`n`). The letters `a`, `b` and `g` sit at positions 8,
9 and 10, which are not single octal digits, so they act like two digits
(`a` = `ny`, `b` = `nn`, `g` = `nw`). That is why `thousand` (`t`,`s`,`a`,`n`)
gives the same code as `tfyn`.

2. Lookup table hidden in a string

`n["1+DIY/.K430x9G(kC["]-42&255^b` compares the word code `b` against a table.
Each character of the string, minus 42 and kept to 8 bits, is the octal code
of one stem: `'1'-42` is 7 (`r` for zero), `'D'-42` is `032` (`tw` for two),
and so on. The position in the string tells what the word means: positions
0 to 9 are the digits, 10 to 15 are the multiplier words, 16 to 18 are ten,
eleven and twelve.

3. Reading past the end of a string

The string is 18 characters long, but the loop starts at index 18. That
index is the hidden NUL (`'\0'`) at the end of every C string, and `(0-42)&255`
is 214, which is octal `0326`: the code for `twl`, meaning twelve.

4. Numbers stored as hexadecimal digits

The total `m` is printed with `%llx`. Each decimal digit of the answer is
kept in its own hex digit, so "two hundred three" is stored as `0x203`, which
prints as `203`. This makes "teen" (`n+16`), "ty" (`n*16`) and multipliers
(shift by a power of 16) easy to compute.

5. Magic number that hides a table of powers

`u=1ll<<6177%n--*4` computes `16` to the power `6177 % (index+1)`. The
number 6177 was chosen so that the remainder gives the right number of zeros
for each multiplier word: `6177%13` is 2 (hundred), `6177%14` is 3
(thousand), `6177%11` is 6 (million), `6177%12` is 9 (billion), `6177%15` is
12 (trillion) and `6177%16` is 1 (`and`, which then has no effect).

6. Multiplying by shifting the low part

`m+=m%u*~-u` adds `(m % u) * (u - 1)`. That moves the digits below `u` up by
`u` while keeping the digits above, so `0x1005` followed by "hundred" becomes
`0x1500`. The `~-u` is a two's complement trick that means `u-1`.

7. Suffix detection with a sentinel value

`r=b%64-25` looks at the last two octal digits of the word. They are `031`
(25, the letters `tn` of "teen") or `030` (24, `ty`). So `r` is 0 for "teen"
and -1 for "ty". `~(int)r` is zero only when `r` is -1, and `!(int)r` is 1
only when `r` is 0, so the ternary chooses between `n*16`, `n+16` or `n`.

8. Longest prefix match by dividing

The loop `for(...;e<47&&b;b/=8)` tries the whole word code first and then
divides by 8, which removes the last letter. This finds the longest known
stem, so `forty`, `fourteen` and `four` all find `fr` and plural words like
`hundreds` still work.

9. One variable doing many jobs

`n` is the index into the letter string in the outer loop, the index into the
table in the inner loop, and the flag that decides whether a new character
must be read (`n++||getchar()`). Because the inner loop always leaves `n` at
0, reading restarts cleanly after each word. `main`'s parameters `e` and `r`
are also reused as ordinary variables, with `r` (a `char **`) holding a small
integer.

10. Case folding with a bit operation

`getchar()|32` sets bit 5, which turns uppercase ASCII letters into lowercase
ones. `EOF` stays negative, so `>=0` still detects the end of input. After
this, every word separator (space, `-`, `,`, `.`, `'`, newline) is below 47,
so `e<47` means "the word ended".

11. Control flow made of loops and operators

There are no `if` or `while` statements. Three nested `for` loops do all the
work, decisions use `?:` and `||` short-circuiting, and the table action is
placed in the increment part of the inner `for` loop so it runs after the body.
