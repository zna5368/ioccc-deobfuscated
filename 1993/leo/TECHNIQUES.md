# 1993/leo

Difficulty: Hard

1. Code laid out as a shape

The whole of `main` is drawn as a big zig-zag (a diamond-like wave) by putting
one or two tokens on each line and indenting them by different amounts. Names
like `main`, `strlen` and `printf` are cut away from their arguments and string
literals are split with backslash-newline (`"No\` / `memory\` / `\n"`), so the
string really is `"No    memory\t\n"` including the spaces and tab of the
indentation. The layout carries no meaning; you have to join the pieces back
together to see the statements.

2. Single letter macros that hold half-statements

`A(x)` is one step of an "add one" with carry, `B` checks one digit and counts a
match, `C` runs `B` four times and then rotates a number by one hex digit, `D`
is the full bulls-and-cows score, `E` is `if(**y)goto`, `F` is
`=a[rand()%c]`, `G` is `unsigned short` and `H` swaps `e` with `a[z]`. Several
of them are not complete expressions: `F` starts with `=` and `D` starts with
`=`, so `h D` means `h = score(...)` and `g F` means `g = a[rand()%c]`. The
`#define D=(...)` and `#define F=a[...]` lines also leave out the space after
the macro name, which looks like a typo but works (the compiler only warns).

3. Mode chosen by the program's name

`*(*y+=strlen(*y)-1)^='d';` moves the `argv[0]` pointer to the last letter of
the program name and XORs it with `'d'`. The letter becomes 0 only when the
name ends with `d`, so from then on `**y` is "false" for `mind` and "true" for
`leo`. Every difference between the guessing game and the solver is controlled
by this one hidden character, for example `*(**y?&g:&e)F` picks either the
computer's first guess `g` or the secret number `e`.

4. Gotos that jump in and out of the same code

The loop body is a chain of labels `q0`..`q3` and `E` (`if(**y)goto`) jumps. In
solver mode the path is `q0` (print guess) -> `q2` (print `??`) -> `q1` (read
the answer) -> `q3`. In player mode the path is `q1` (read the guess) -> `q2` ->
print the score -> `q3`. Both modes share the same `printf` and `scanf` calls,
so the order of the printed text only makes sense once you follow both paths.

5. Numbers packed into hex digits

A code of four colors is stored in one `unsigned short`, one color in each hex
digit (4 bits). Colors are stored as 0..n-1 and shown as 1..n by adding `4369`,
which is `0x1111` (one added to every digit). The guess is read with `scanf("%hx")`
so typing `1234` reads the hex number `0x1234`, and the answer `12` is read as
`0x12`, which is "1 bull, 2 cows" in two hex digits. `h!=64` stops the game
because 64 is `0x40`, meaning 4 bulls.

6. Counting in base n with bit tricks

The list of all possible codes is built by adding 1 to the previous code and
fixing the carry with `A(A(A(A(0))))`. `(e^d)&i` is zero when the digit under
the mask `i` equals the number of colors `d`, which means it overflowed. Then
`e+=i+i/15-d` clears that digit and adds one to the next: for `i=0xF`,
`i+i/15` is `0x10`. `d<<=4,i<<=4` move on to the next digit. The innermost
`A(0)` gives 0 if every digit overflowed.

7. Bulls and cows by rotating the guess

`D` copies the guess into `d` and runs `C` four times. The first `C` compares
each digit of `e` with the same digit of `d`: these are the bulls, and `b<<=4`
moves them into the upper hex digit of the result. After each `C`, `d=d<<4|d>>12`
rotates the guess by one digit, so the next three `C`s compare every digit with
the guess digits in other positions: these are the cows. When a guess digit
matches, `d|=i` turns it into `F`, a value no color can have, so it is never
counted twice. `b+=!(...)&&(d|=i)` uses `&&` so the marking only happens on a
match.

8. Loop bodies hidden in the loop condition

Both `for` loops have an empty body; all the work is in the condition. The first
one fills `a[]` with the comma expression `(e=a[c-1]+1,a[c]=...,1)`, where the
final `,1` keeps the condition true. The filter loop `(z<c||(c=f,0))&&(...)` uses
`||` to run `c=f` (save the number of codes kept) only when the loop is about to
end, and `b||(a[f++]=a[z])` to keep a code only when its score matches the
answer. The `do...while` condition also hides the next guess
`((g F),1)` behind `&&`.

9. XOR swap used to reuse the scoring macro

The score macro `D` always compares `e` with `d`. To score candidate `a[z]`
instead of the secret, `H` swaps `e` and `a[z]` with three XORs
(`e^=a[z],a[z]^=e,e^=a[z]`), computes the score, and swaps back with `H` again.

10. Meaningless variable names and reused variables

All variables are single letters, and some have more than one job. `z` is first
`argc`, then the number of colors and finally the loop index of the filter. `e`
is the secret number in `mind`, a temporary while building the code list, and a
swap slot while filtering. `c` counts the remaining candidates and `b` is the
score.
