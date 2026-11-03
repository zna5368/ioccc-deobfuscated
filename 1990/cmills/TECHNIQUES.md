# 1990/cmills

Difficulty: Medium

1. Interleaved string table

All nine text strings (the prompts, the messages and the card ranks
`A23456789TJQK`) are stored in one string `m`, woven together like the
columns of a table. String number `n` is made of the characters at positions
`n`, `n+9`, `n+18`, ... until a `\0`. The function `b(o,n)` walks through `m`
in steps of 9 and copies string `n` into the buffer `o`, so `b(l,3)` builds
`"Total $%d.  Wager?"` and `b(c,2)` builds the list of card ranks. None of the
messages can be read in the source. `simple.c` writes each string out in full
where it is used.

2. Macros that hide statements

Short macros stand for whole pieces of code. `H(v,g)` deals a card into a hand
and adds it to the total, `S(q)` prints the table, `B(n)` decodes message `n`
and starts a `printf` call, and `F` is the start of a `for` loop over the shoe.
`W(w)` is the strangest: it contains an `if`, an `else` and then ends with a
second `else`, so the code after `W(...)` becomes the else branch of an `if`
that is hidden inside the macro. `simple.c` replaces these with the functions
`dealer_hit`, `player_hit`, `show` and plain `if`/`else` blocks.

3. Settings passed in from the Makefile

`M`, `N`, `X` and `V` are never defined in the source; the Makefile passes them
with `-D`. `M` is the largest bet but is also used as the size of the text
buffers and to set the starting cash (`M*2`), `N` is the size of the shoe, `X`
is the call that seeds `rand()` and `V` is just `void`. `simple.c` gives them
names such as `MAX_BET`, `LINE_SIZE` and `SHOE_SIZE`.

4. Negative numbers mean a soft hand

A hand total is a positive number for a "hard" hand, and a negative number for
a "soft" hand (one that holds an ace counted as 11). The function `h` adds a
card and flips the sign or subtracts 10 when an ace has to change from 11 to 1.
This lets the program test for blackjack with `x==-21` and make the dealer
stand on soft 17 with `x>-17&&x<17`, but it looks like nonsense until you know
the rule. `simple.c` calls the function `add_card` and explains the sign.

5. Backwards array indexing and a hidden unary minus

C lets you write `a[i]` as `i[a]`, so the code uses `1[z]` for `argv[1]`,
`1[q]` for the dealer's hole card and `2[g]` for `g[2]`. The split test
`*p==-1[e]` looks like "the first card equals the last card" (`e[-1]`), but it
is really `-(e[1])`, the minus sign applied to the character after the hand.
That can never equal a card, so a pair can only be split when it is worth a hard
20. `simple.c` keeps the real meaning and adds a comment about it.

6. Many hands packed into one string

After a split, every hand the player has is kept in the one string `p`. Split
cards that are still waiting are at the front, each followed by a space, and the
hand being played is at the end. Splitting shifts the whole string two places
right in place (`for(g=e++;2[g]=*g,g!=p;)--g;`), and moving to the next hand
copies the first waiting card to the end and shifts everything two places left.
`simple.c` puts these steps in `split_hand` and `next_split_hand`.

7. Jumping into the middle of a loop with goto

The label `i:` is inside the body of the split `while` loop. When a split hand
is finished, `goto i` jumps back into that loop to deal the next hand, and
`goto _` jumps forward to the payout from blackjacks and split aces. This makes
the order of play very hard to follow. `simple.c` uses a normal `for` loop over
the hands and a flag for split aces.

8. Expressions used instead of statements

Operators are used where an `if` would be clearer. `x<0&&(x=-x)` takes the
absolute value only when `x` is negative, `b(c,2)*X` multiplies two calls just
so both run in one statement, and insurance is paid with
`y+=(3*(h(0,1[q])==10)-1)*w/2`, which adds the bet when the hole card is worth
10 and subtracts half the bet otherwise. `simple.c` writes these as `if`
statements.

9. Variables reused for different jobs

Every variable has a one letter name and several are used for more than one
thing. `w` is the argument count, then the bet, then a temporary while
shuffling. `l` holds both the decoded message to print and the line typed by
the user. `g` points into the dealer's hand but is also used when shuffling and
splitting, and `f` is the "doubled down" flag that is also bumped during the
payout. `simple.c` uses separate, named variables.

10. Recursion instead of a loop

The yes/no question function `u` calls itself again when the answer does not
start with `y` or `n`, with the whole decision packed into one `return`
expression of `||` and `&&`. `simple.c` uses a `for` loop in `ask`.

11. Old style C

The program uses K&R function definitions (`b(o,n)char*o;{...}`), functions
with no return type, `exit()` declared as `V exit();`, and calls `atoi`, `rand`
and `srand` without including the header that declares them. The original also
redefines `gets` as a macro that calls `fgets`. `simple.c` uses `fgets`
directly.
