# 1993/ant

Difficulty: hard

1. Keyword-hiding macros

`#define W while`, `#define Q if`, `#define E else`, `#define R return` and
`#define G fprintf(stderr,` replace C keywords and a common function call with
single letters. The code still has normal structure, but you have to translate
each letter back in your head while reading. The simplified version just uses
the real keywords.

2. Macros that hide pointer dereferences

`#define B **p` and `#define D ++*p` hide the fact that `p` is a pointer to
the pattern string pointer. `B` reads the current pattern character and `D`
moves to the next one. Even parameter declarations use the macro: `C B;`
expands to `char **p;`. In simple.c these are written out as `**pattern` and
`++*pattern`.

3. Typedefs for basic types

`typedef int I; typedef char C; typedef void V;` turn every declaration into
one-letter types, so `I d[A][A];` is really `int d[512][512];`. Together with
the one-letter names, nothing on the page hints at what a variable holds.

4. Single-letter names for everything

Functions are named `N`, `X`, `T`, `F`, `L`, `O`, `P`, `U` and `M`, and the
globals are `a`, `b`, `d`, `m`, `n`, `o`, `x`, `z`, `f`. They were renamed to
what they actually do: `new_element`, `expr`, `term`, `factor`, `literal`,
`add_reachable`, `in_class`, `can_reach_final`, `match`, and `nfa`,
`nfa_length`, `dfa`, `set`, `new_set`, `next_set`, `nesting`, `current_char`,
`reached_final`.

5. Magic negative numbers as instructions

The pattern is compiled into a plain integer array where positive values are
characters to match and negative values are hidden commands: `0` means "any
character", `-1` and `-2` start a negated or normal `[...]` class, `-3` is an
empty step, and `-4-n` means "jump to element n". Expressions such as
`a[i] = -4-b` or `O(-4-a[p])` only make sense once you know this encoding.
simple.c gives each value a name (`WILD`, `NCLASS`, `CLASS`, `PASS`) and uses
`JUMP(n)` / `JUMP_TARGET(x)` macros for the jump encoding.

6. One value with several meanings

The newline character `'\n'` is used as the beginning-of-line anchor, the
end-of-line anchor and the terminator of a character class list. `main()`
stores a `'\n'` in front of each line (`*l = '\n'`) and `fgets` leaves one at
the end, so the anchors work by matching these real newline characters. The
value `2` in `m[]` is also used as a separator between DFA state sets because
every set starts with element 2 (the "any character" of a hidden `.*` that is
placed in front of the pattern). simple.c names these `BOL`, `EOL`, `STOP` and
`SET_DELIMITER`.

7. Hidden setup with unexplained calls

`N(-3); N(-6-b); N(0); N(-3-b);` in `main()` silently builds a `.*` loop before
the user's pattern so that a match can start anywhere in the line. Because the
jump targets are computed from the current length `b`, you have to trace the
array by hand to see that this is "skip, loop over any character".

8. Packed boolean expressions and side effects in conditions

`U()` is a single `return` statement made of nested `||` and `&&` that
recursively follows jumps. `P()` updates the caller's index through a pointer
while scanning a class, and computes `(j = a[*p] == -1)` inside an `if`
condition, then returns `i ^ j` to flip the result for negated classes.
`c = d[c][*p] = f ? -1 : k;` both stores a new DFA transition and changes the
current state in one line, with `-1` meaning "matched".

9. Lazy DFA construction squeezed into one loop

`M()` builds DFA states on demand. All state sets are stored back to back in
one array `m[]`, and a hand-written comparison loop with two indexes (`i`
and `j`) and a counter `k` searches the earlier sets for a duplicate of the
newly built one. There are no helper functions or comments, so this compact
loop is the hardest part to follow. simple.c keeps the algorithm but adds
comments explaining each phase.

10. K&R style function definitions

Functions are declared in the old style (`I N(c) I c; {...}`) with implicit
prototypes `I N();`, which hides the parameter types from the reader. simple.c
uses ANSI prototypes.
