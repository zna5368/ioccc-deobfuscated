# 1985/sicherman

Difficulty: Medium

1. Macro arguments that are thrown away

`#define _C_C(_)('\b'b'\b'>=C_C>'\t'b'\n')` takes a parameter named `_`, but
the body never uses that parameter directly. It uses `C_C`, another macro that
expands to `_|_`, and that expansion happens after the argument would have been
substituted. So whatever you pass to `_C_C(...)` is silently dropped and the
`_` in the result refers to the variable `_` instead. This is how the call
`subr(&v)` at the end of `main` disappears: it sits inside the argument of a
`_C_C(` and is never compiled. `subr` looks important but is dead code.

2. A macro that opens a parenthesis it does not close

`#define v _C_C(` expands to a macro name followed by an opening parenthesis.
The preprocessor then grabs the following text, up to the matching `)`, as the
argument. In `(_=C_C_(__), v))` the argument is empty; in `_=c-v+subr(&v));`
the argument is `+subr(&v)` and the extra `)` that looked like part of the
`while` statement is actually the end of the macro call.

3. Comments used as tokens

`#define c /**/` makes `c` expand to an empty comment, so `_=c-v...` is just
`_=-v...`. The comment `/* C program. ... (In the */C, Manual)` in the
parameter list of `main` hides the real parameter list in the middle of English
text, making it look like prose. The string in `subr` and the multi-character
constant `'/*"'` contain `/*` to confuse preprocessors that strip comments
badly (the judges mention some `cpp`s failing on this).

4. Single-letter macros for operators

`#define b *` lets `'\b'b'\b'` read like a strange character literal, but it
is `'\b' * '\b'` which is 8 * 8 = 64 (the character before `'A'`). Likewise
`'\t'b'\n'` is 9 * 10 = 90 (`'Z'`). These two numbers are the bounds used to
test whether a character is a letter.

5. Character constants used as numbers

`~' '` is the bitmask that clears bit 0x20, so `C_C_(__)` (`~' '&__`) turns a
lowercase letter into uppercase. `('\b'b'\b'+~' '&'\t'b'\n')` relies on `+`
binding tighter than `&`: it is `(64 - 33) & 90`, which is 26, the number of
letters in the alphabet. The constant `52` is `'A' - 13`, so
`(_-52)%26+1` gives the 1-based alphabet position of the letter 13 places on.

6. Variables named `_` and `__`, with shadowing

The only variables are `_` and `__`. They are declared both globally and again
locally inside `main`, so the globals (used only by the dead `subr`) are hidden.
`__` holds the input byte and `_` is reused for the uppercase copy, the
"not a letter" flag, and finally the output byte.

7. Everything packed into the loop condition

`while (read(...) & write((_=(_=..., test)) ? __ : rotated), &_, 1))` reads a
byte, computes the ROT13 result inside the arguments of `write`, and writes it,
all in the condition. The comma operator stores the uppercase letter before the
test, and the nested assignment stores the result. Using `&` instead of `&&`
means `write` runs even when `read` returns 0 at end of file, so the last
character is written twice. That is why piping the program through itself does
not give back exactly the original file.

8. Misleading names and messages

The parameter of `main` is called `Manual` (it is really the argument count)
and `subr` assigns a string about `lint` complaining that `Manual` is unused.
Neither has any effect on the output; they are there to distract and to set up
the joke in the comment ("If you don't understand it look it up. In the C
Manual").
