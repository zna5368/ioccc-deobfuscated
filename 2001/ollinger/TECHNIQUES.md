# 2001/ollinger

Difficulty: Hard

1. Encoded transition table

The 9-state automaton's rules are packed into the string `e`. Each rule is one
character whose value minus 42 holds two digits in base 9: `(ch-42)/9` is the
state of the right neighbour the rule applies to, and `(ch-42)%9` is the new
state of the cell. Picking the offset 42 keeps every character printable
(between 32 and 127). In simple.c the string is decoded into a plain
`next_state[left][middle][right]` array.

2. Index table with arithmetic correction

The last 81 characters of `e` (from `e[345]`) are a second table giving where
the rules for each `left*9+middle` pair start. The stored character alone cannot
reach every offset while staying printable, so the code fixes it up with
`+ b/19*85 - 33 - b/40*12 - b/80*4`. Integer division makes each term switch
on only after `b` passes 19, 40 or 80, adding a different offset for different
parts of the table.

3. Unrolled search hidden in a macro

`#define n ((e[++a]-42)/9-f[d+1])?` is repeated eight times to build one long
chain of `?:` operators. Each step moves `a` to the next rule and tests if its
right-neighbour digit equals `f[d+1]`. The first match is used; if none of the
nine rules match, the chain gives `0`, and `e[0]` is the character for "state 0".
This is a linear search written as one giant expression. simple.c replaces it
with a direct array lookup.

4. Output macro and character-indexed string

`#define o printf("%c",c[" 01./:;|\\"]);` hides a print statement. It also uses
`c[string]`, which is the same as `string[c]` because `a[b]` means `*(a+b)`.
The same trick appears as `0[f]`, `1[k]`, `d[f]` and `(...)[e]`.

5. Assignments buried inside expressions

`g=0[f=(char *)calloc(80+(h=atoi(1[k])),1)]=1` reads the argument into `h`,
allocates `f`, sets `f[0]` to 1 and sets `g` to 1, all in a single `for`
initialiser. It looks like `g` starts at 0, but it actually starts at 1. In the
same way `b=c*9,b+=(c=d[f])` uses the comma operator to build the rule index and
update `c` inside an array subscript.

6. Previous cell kept in a reused variable

The grid is updated in place, so the old value of the left neighbour would be
lost. The code keeps it in `c`, which is also the value being printed. Because
the print happens before `c` is updated, the printed value lags one cell
behind: each row starts with a blank and needs one extra print after the loop. simple.c prints the row first
and then updates it with a separate `left` variable.

7. Meaningless names and missing headers

Variables are single letters (`a`, `b`, `c`, `d`, `e`, `f`, ...) and
`printf`, `calloc` and `atoi` are used without including their headers, relying
on old implicit declaration rules.
