# 1992/buzzard.2

Difficulty: hard

1. Macros that hide statements

`#define c m[m[0]++] =` makes `c x` mean "store `x` at the end of the
dictionary and move the end forward". It looks like a function call with no
parentheses, so `c 2,c atoi(s)` is hard to recognise as two writes to memory.
`#define z;break;case` hides the `break` at the end of every case, so the
`switch` reads as a list of `z 5:`, `z 12:` with no visible `break`. It also
puts an empty statement and a useless `break` right after `switch(...){`.
`simple.c` uses a `compile(value)` macro and writes out each `case` and
`break`.

2. One-letter names and bare opcode numbers

All variables are single letters: `m` is main memory, `s` holds the names of
words, `L` is the most recently defined word, `I` is the instruction pointer,
`T`/`S` are the data stack and its pointer, `f` is the top of the stack and
`t` is where the next name is stored. The functions are `a` (create a word)
and `r` (run an opcode). The opcodes are plain numbers 0 to 15 with no hint of
what they do. `simple.c` gives every variable a name and every opcode a
`#define` such as `OP_FETCH` or `OP_CALL`.

3. Shuffled switch cases

The cases in `r()` are in the order 5, 12, 15, 1, 9, 7, 0, 8, 2, 11, 4, 6, 10,
3, 13, 14, so related operations are not next to each other. Because every case
ends in a `break` (hidden by the `z` macro) the order does not matter, but it
makes the reader jump around to understand the program.

4. Important pointers kept inside the memory array

`m[0]` is the end of the dictionary and `m[1]` is the top of the return stack.
They are written as `*m` and `m[1]` instead of having their own variable
names. They are kept in memory on purpose so that programs written in the
language can read and change them with `@` and `!`, which is how the `third`
file builds a full language on top. `simple.c` keeps them in the same place
but gives them the names `here` and `return_sp` with `#define`.

5. A cached top of stack

The top item of the data stack is not stored in the stack array but in the
variable `f`. So "pop" is written `f = *S--` and "push" is `*++S = f`, and an
operation like subtract is `f = *S --- f`, which is really `f = *S-- - f`
written without spaces so that it looks like a `---` operator. `simple.c`
names the variable `top` and spaces the operators out.

6. Hidden constants in memory

Numbers are compiled as `c 2, c atoi(s)`. The `2` is not an opcode but the
address of `m[2]`, which is never written and so is always 0, the
"push literal" opcode. The search for a word starts at `L` and follows links
until the name matches. The list ends with the link value 1, and since
`m[1+1]` is `m[2]` which is 0, the end of the list has the name `&s[0]`,
which is the word just read. So the search always finds a match and stops at
`w == 1` for unknown words, then `w-1 ? ... : ...` treats it as a number.
`simple.c` names these `LITERAL_ADDRESS` and `END_OF_DICTIONARY` and explains
them with comments.

7. Expressions used instead of statements

`for(w=scanf("%s",s)<1?exit(0),0:L; ...)` hides a "stop at end of input" check
inside the start of a `for` loop using `?:` and the comma operator. The result
of the lookup is handled with `w-1 ? r(w+2) : (c 2,c atoi(s))` instead of an
`if`. `main` uses `a(1),c w++` in a `for` with no increment section, and the
main loop is `for(*m+=512;;r(m[I++]));` with all its work in the header.
`simple.c` splits these into ordinary `if` statements and loop bodies.

8. Self-modifying dictionary entries

Every word has a code field that says what happens when it is read: words
are compiled by default (opcode 1), so the language is always compiling. The
`immediate` primitive (opcode 4) does `*m-=2;c 2`, which steps back over the
word just being defined and overwrites its code field with the "call" opcode
2, so the word runs as soon as it is read instead of being compiled. The main
loop is also built in memory instead of in C: `c w; c I-1` makes two cells,
one that runs `_read` and one that calls itself again, which loops forever.

9. Names of the primitives read from the input

The program does not contain the names of its own commands. `main` calls
`a(...)` thirteen times and each call reads the next word of input as the name
of a primitive, so the meaning of `@`, `!`, `-`, `exit` and the rest only
appears in the first line of the `third` file.
