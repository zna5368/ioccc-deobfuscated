# 2004/burley

Difficulty: Hard

1. Whole program in one expression

`main` is a single `return main(argc, (char*)( ... ));` statement. Everything in
between is one huge expression joined by the comma operator, which evaluates
its operands left to right and throws away all but the last value. The recursive
call to `main` never actually happens, because the expression loops forever
and never finishes.

2. Loops built from `setjmp`/`longjmp`

There are no `for`, `while` or `goto` statements. `setjmp(p[n])` saves a spot in
the program and `longjmp(p[n], x)` jumps back to it, so a pair of them acts as
the top and bottom of a loop. Only four jump buffers are reused for every
nested loop: for example `setjmp(p[0])` is the deal-retry loop, the display loop
and the flush check at different times. You have to track which `setjmp` last
filled each buffer to know where a `longjmp` will land. The outer game loop is
`setjmp(p[2])` ... `longjmp(p[2],3)`.

3. `&&` and `||` instead of `if`

Conditions are written as `cond && (body, 0)` or `x || y`. Because `&&` only
evaluates its right side when the left is true, it works like `if`. For example
`b||++b` means "if the bet is 0, make it 1", and
`e[s]&(1<<t)&&(longjmp(p[0],0),0)` means "if this card was already dealt, pick
again". The `, 0` gives the `longjmp` call a value so it can be used in the
expression.

4. Encoded string tables

Rank letters are printed as `"(.7-A,G+@*J)/"[(rank)*013%13] + 012`. Each
character is 10 less than the real one, which gives `"28A7K6Q5J4T39"`, and the
index is scrambled by multiplying by 11 modulo 13. Put back in order the table
is just `"23456789TJQKA"`. The payout table `"AACEDJQZHello World"[s]-'A'`
hides the multipliers 0, 0, 2, 4, 3, 9, 16, 25 as letters, with
"Hello World" as unused padding.

5. Strange ways to write simple numbers

Constants are written in octal, hex, shifts or character tricks: `017` and
`0xf` are 15, `013` is 11, `012` is the newline, `1<<037` is the top bit,
`4>>3` is 0, `0x1F+!!putchar(...)` is a space (31 + 1) and
`"\004"["\004"[1]]` is `"\004"[0]`, which is 4. Assignments are hidden as well:
`j+=-j` sets `j` to 0, `h+=*d++-h` sets `h` to `*d++`, `g[j]^=g[j]` sets `g[j]`
to 0 and `e[i+j]^=*(i-j+e+(j<<1))` is `e[i+j]^=e[i+j]`, which clears it.

6. Reusing the same variables for everything

The single-letter globals `h i j k s t` change meaning all the time. `k` is the
bet, then the keep mask, then a loop counter, then a rank difference. `s` is a
suit and later the score. `i` is a bit counter, a card index, the flush test
result and the pair-scan weight. `h` holds the time from `srand`, then each
input character.

7. Pointer tricks for the first deal

The first and second deals share one loop that parses the "cards to keep" text
through the pointer `d`. On the first pass `d` points at the bytes of the
integer `j`, which is 0, so it reads an empty string and keeps nothing. On the
second pass `d` points at the line read with `fgets`. Each digit is turned into
a bit by counting `h` down while shifting `i` from 16 down to 1. Then
`k^=-1, k+=1<<037` flips the mask so it marks the cards to replace.

8. Hidden hand ranking

Poker hands are not tested one by one. The flush test is a backwards scan that
leaves `i` at -1 if all suits match. The straight test leaves `j` at 0, and
`8==-k` handles the A-5-4-3-2 straight. Then `(j*=i*=4)&&(i=1)` folds both
results into `i` and `j`. Pairs are counted in two passes over the sorted hand.
The first pass zeroes every card that does not match its neighbour, so the
second pass finds only three or four of a kind, and it counts double. The total
selects a letter from the payout table, and `f` remembers whether a pair was
jacks or higher. One more formula adds the flush bonus and gives a royal
flush 250 times the bet by checking that the middle card is a queen
(`(g[2]&017)^0xa?8:-192`).

9. Side effects in function arguments

Output calls are nested so that one statement prints two characters, as in
`putchar(0x20+!putchar(...))`. The inner `putchar` prints the rank, and since it
returns a non-zero character, `!` turns that into 0 and the outer call prints a
space. `srand(time((time_t*)&h))` writes the time into the variable `h` as a
side effect, and the bet is read with
`atoi((fgets(a,...), a+strspn(a," -")))`, which hides the `fgets` inside the
`atoi` argument.

10. Out of bounds and type punning

`char**D` is cast to `char*d` and later points into an `int`. The loop that
clears the used-card masks runs `i` through 0, 1 and 3, so it also writes
`e[4]`, which is one past the end of `e` (in practice it clears `g[0]`).
`time` writes a full `time_t` into the `int h`. These only work because the
program is built with `-O0`.
