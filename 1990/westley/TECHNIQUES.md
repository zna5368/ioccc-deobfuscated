# 1990/westley

Difficulty: Medium

1. Variable names chosen to read as English

The names `lie`, `time`, `me`, `not`, `rested`, `get`, `out`, `ly`, `die`,
`lotte`, `dear`, `my`, `let`, `love`, `hell`, `eroticism`, `that` and `this`
are picked so that code like `char lotte` ("charlotte"), `double time, me`
and `main(ly, die)` ("mainly die") reads like a letter. The names say
nothing about what the variables hold: `ly` is `argc`, `die` is `argv`,
`lotte` is the position in the message, `get` remembers whether the next
answer is "loves me", `not` counts the petals left, and `lie` points into
the message. Worse, `not` and `me` are `double`s used as plain counters
and flags.

2. Shadowed variables

`lotte`, `get` and `my` are declared again in inner blocks (`char lotte,
my= dear` and `char*les, get= 0xFF, my;`). An inner declaration hides the
outer one with the same name, so a reader cannot tell which variable a
name means without carefully matching braces. The inner `get` inside the
`switch` is never even initialized, because the jump to a `case` label
skips its initializer.

3. Statements that do nothing

Most lines are expressions whose value is thrown away, written only to
make the text read as prose: `1 - out & out ;lie;`, `(char)*lie;`,
`get *out* (short)ly -0-'R'- get- 'a'^rested;`, `!not +(char)*lie*'s';`,
`0xF +(char*)lie;`, `get - !out;` and `auto*eroticism, that;` (two unused
variables). Even `hell:0, (char)*lie;` is a label followed by a useless
comma expression. Deleting all of them does not change the program.

4. Loops that run exactly once or never

`for(get= !me;; not){ ... break;}` runs its body once and exits with
`break`. `do { ... } while(!"you're at it")` runs once, because a string
literal is a non-null pointer, so `!"..."` is 0. `for(;!'a';);` never runs
since `'a'` is non-zero. `do { ... } while(love (char*)lie)` expands to
`while((char*)lie - (char*)lie)`, which is always 0. The inner
`do { ... } while( my - dear && ...)` also runs once because `my` was just
set equal to `dear`.

5. Constants written as arithmetic on character literals

Simple numbers are spelled as expressions on letters so their value is
hidden. `out - 'c' -('P'-'S') + die + -2` is `97 - 99 + 3 - 2 = -1`, so
`puts(*(... + die ...))` is just `puts(argv[0])`. In
`*((char*)&lotte) -= '4' - ('I'-1l)` the amount is `52 - 72 = -20`, so it
actually adds 20. `!0XFACE` is 0 and `!!0xBABE` is 1. `'I'-1l **die[2]`
looks like a strange pointer expression but is `'I' - 1 * *die[2]`.

6. A macro that hides pointer arithmetic

`#define love (char*)lie -` is defined in the middle of a function. Every
`love x` becomes `(char*)lie - x`, so `(love ly)[(char)++lotte + !!0xBABE]`
is `(lie - argc)[lotte + 1]`. With the expected two arguments this is
`lie[lotte - 1]`, the character just before the one about to be printed.
The loop test therefore checks the previous character, and the program
also prints the `'\0'` that ends `"snot\n"`.

7. Values hidden in multiple assignments and conditionals

`not= atoi(let[...]) - !!(time =out= 'a')` sets three variables in one
line: `out` and `time` become `'a'`, and `not` becomes the number of
petals minus one (`!!'a'` is 1). The index of `let` is a `?:` whose true
branch `(char)lotte-(char)lotte` is always 0, and the long false branch
is never taken, so it is just `argv[1]`. `**let= !!me *!not+ ++die`
is `0 + ++die`, which also moves `die` from `argv` to `argv + 1`.

8. Message cut from a decoy sentence

The only string is the insult `"The gloves are OFF this time, I detest
you, snot\n\0sed GEEK!"`. The program never prints it in order. It starts
after `"The "`, prints `"loves "`, jumps 15 characters at every space
(`case' ': *((char*)&lotte) += 15;`) to land on `"me"`, and either jumps
another 20 from the comma to the `'\n'` or keeps going through `", "` to
`"not\n"`. The `\0sed GEEK!` part after the newline is never printed.

9. Flag toggled inside a loop that never runs

`for(get=! get; !out; ...) return !!(char)lotte;` looks like an early
return. But `out` is `'a'`, so `!out` is false and the body never runs.
The only work done is the init part `get = !get`, which flips between
"loves me" and "loves me, not" on each petal.

10. Accidental reads past the end of argv

The test `'I'-1l **die[ 2 ]` reads `argv[3]` (since `die` was moved to
`argv + 1`). With one argument, `argv[2]` is the terminating NULL and
`argv[3]` is past it, which on Unix systems is the first environment
string. The "loves me" jump only happens when that string does not start
with `'I'`, and the program crashes with an empty environment.

11. Backward goto as the outer loop

`hell:` is placed in the middle of a statement list and `if (not--) goto
hell;` at the end jumps back to it. This is a `do`/`while` loop over the
petals written as a label and a `goto`, with the counter being a `double`.

12. Exit status from a leftover counter

`exit( (char)lotte)` returns the final message position (46) instead of
0, so the program always appears to fail even when it worked.
