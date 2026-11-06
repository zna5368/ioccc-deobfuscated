# 2024/howe

Difficulty: Hard

1. Accented and non-Latin identifiers

C allows Unicode letters in names, so the author used look-alike names such as
`ó`, `ò` and `ö` for variables and `Û`, `Ê`, `Ë`, `É`, `È`, `Ô`, `Ó`, `Í`,
`Â`, `Ñ`, `ө` (Cyrillic), `κ` (Greek), `Ф`, `Ќ`, `Κ` and even `🔥` for
functions. `ó`/`ò` sit next to `o` (the cursor) and `Ê`/`Ë`/`É`/`È` look
almost the same, so it is very hard to tell which one is being used. `Κ`
(Greek capital kappa, the command table) looks exactly like the Latin `K`
(the "cursor up" function). In simple.c they became `undo_gap_start`,
`undo_gap_end`, `undo_point`, `cmd_undo`, `cmd_page_top`, `cmd_page_bottom`,
`cmd_page_down`, `cmd_page_up`, `cmd_yank`, `cmd_delete_char`, `cmd_paste`,
`cmd_toggle_selection`, `cmd_search`, `cmd_goto_line`, `cmd_write`,
`cmd_quit`, `command_keys`, `command_table` and `cmd_redraw`.

2. Single letter names for everything else

Every other global and function is one letter: `b g h c` are the buffer, the
start and end of the gap and the end of the buffer; `o u v` are the cursor,
the top and the bottom of the page; `d` is the selection mark, `q`/`Q` the
clipboard and its length, `z` the repeat count, `x`/`y` the cursor's screen
column and row. Functions are named after the vi key that runs them (`H J K L
B W X I F`) or just a random letter (`P Z S E D V G O A M N C Y T`), and one
function is called `_`. Each letter was replaced by a descriptive name such as
`ptr_to_pos`, `pos_to_ptr`, `utf8_length`, `next_char`, `prev_char`,
`move_gap`, `char_width`, `line_start`, `advance_to_column`, `row_start`,
`prev_row`, `next_row`, `pad_line`, `display` and `execute_command`.

3. Names reused for different things

`main(int i, char **j)` reuses its `argc` parameter as the file descriptor,
and `v` is the "end of page" position but is also used as a temporary for
swapping in `Û()` and to remember the old page top in `Y()`. Inside `Y()`
the loop declares `int t = S(*p);` right after the line `v < t` which still
refers to the outer `long t` (the selection end), because a new variable only
exists from the point where it is declared. simple.c gives each of these its
own variable (`fd`, `old_top`, `sel_end`, `length`).

4. Hidden headers and iso646 operator words

The source has no `#include` lines at all; the Makefile adds nine headers with
`-include`, including `iso646.h`, which defines words for operators. The code
then writes `and`, `or`, `not`, `not_eq`, `bitand`, `bitor` and `xor_eq`
instead of `&&`, `||`, `!`, `!=`, `&`, `|` and `^=`, so expressions read like
English but are easy to misread. simple.c uses the normal operators and
explains in a comment where the headers come from.

5. Magic numbers

UTF-8 handling is done with bare decimal numbers: `S()` returns
`1+(a > 193)+(a > 223)+(a > 239)` and continuation bytes are found with
`(192 bitand c) == 128`. These are the hex values `0xC2`, `0xE0`, `0xF0`,
`0xC0` and `0x80` that mark 2, 3 and 4 byte sequences and continuation bytes.
Key codes `3`, `8` and `27` are CTRL+C, backspace and escape, and `T(14)` /
`T(99)` limit the commands to the first 14 motions or allow all of them. They
are now named constants such as `CTRL_C`, `BACKSPACE`, `ESCAPE`,
`MOTION_COMMANDS` and `ALL_COMMANDS`.

6. Command dispatch through a parallel string and function table

There is no `switch` on the key. Instead `Ќ[] = "hjklbwHJKL|G/nixydPu\\WQ\003"`
is searched for the key, and the position found is used as an index into
the function pointer array `Κ[]`. Any key that is not found lands on the
string's terminating NUL, which is the last entry `🔥` (redraw). Because the
first 14 entries are motions, the delete/yank command calls `T(14)` to read
one more key, and only a motion is accepted. This makes `d` + motion work
without any code that mentions motions.

7. Buffer gap with clever undo

The text is stored with an empty "gap" at the cursor. `P()` and `Z()` convert
between pointers and positions using the expression
`s-b - (s < h ? 0 : h-g)`, which is hard to read without knowing the trick.
Deleting only moves the gap end (`h += Q`) and inserting only writes into the
gap, so the old text is never erased. `V()` saves the gap before each edit
and `Û()` swaps the saved and current gap, which undoes (and redoes) the last
edit with three pointer swaps. Searching moves the gap to the end and writes a
`'\0'` into it so the whole text becomes one C string for `regexec`, and the
search prompt is even typed into the unused gap memory.

8. Packed expressions and side effects

Many lines do several things at once: `O()` returns `(m+1) * (0 < m)` instead
of using an `if`; `ө()` starts at `o = n * (not z)` to pick the end of file or
the top; `T()` merges two counts with
`z = j and z ? j*z : z ? z : j;`; `Y()` puts `standend()` inside the loop
condition with a comma; `if (i++ < LINES)` tests one row and draws on the
next; and `È()` counts down the global `y` with `1 < y--`. The swap in `Ô()`
uses the XOR trick `i xor_eq o; o xor_eq i; i xor_eq o;`. In simple.c these
are written as plain `if` statements, loops and a temporary variable.

9. Assignments hidden inside conditions and calls

`open(f = *++j, 0)`, `write(i = creat(f,MODE), ...)`, `q = strndup(h, Q =
i-o)`, `(a = wcwidth(t)) < 1 ? 1 : a` and `while (m < n and (p = Z(m =
E(m))) < c)` set variables in the middle of other expressions, so it is easy
to miss that a value changed. simple.c moves each assignment to its own line.
