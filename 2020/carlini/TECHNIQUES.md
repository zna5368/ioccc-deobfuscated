# 2020/carlini

Difficulty: Catastrophic

1. The whole program is one `printf` call

`main` is just `while(*d) printf(fmt, arg);`. All game logic (checking wins,
ties, illegal moves, drawing X and O) is hidden inside the format string. In
`simple.c` each part of that format string is turned into a normal C function
call such as `add(253)` or `store(temp)`.

2. Using `%n` to write to memory

`%hhn` does not print anything. It stores the number of characters printed so
far into a byte. By printing an exact number of characters first, the program
can put any value it wants into memory. For example `%10$10d%39$hhn` prints 10
characters and then writes 10 (a newline) into the board text. `simple.c`
calls this `count_up_to('\n'); store(...)`.

3. Arithmetic by counting printed characters

`%10$253d` prints the number 0 padded to 253 characters, so it adds 253 to the
count. Because only the count mod 256 is stored, adding 255 is the same as
subtracting 1. `simple.c` names this `add(width)`.

4. Bits stored as tiny strings

A true value is a byte that is not zero followed by a zero byte, and a false
value is two zero bytes. Printing one with `%s` adds 1 or 0 to the count.
Printing a bit 47 times multiplies it by 47. This is how the board cells
become `' '`, `'X'` (32 + 56) or `'O'` (32 + 47), and how the status text
becomes `"WINS"` or `"TIES"`. See `add_bit` and `add_bit_times`.

5. Logic gates made from printf

Printing three bits and adding 253 gives 0 only when all three are true (a
NAND). Printing one bit and adding 255 gives a NOT. Printing two bits and
storing gives an OR. These are the `nand3`, `not` and `or` functions, and the
win, tie and illegal move checks are wired together from them.

6. Resetting the counter to zero

The macro `R` repeats `%12$hhn%12$s` 256 times. Each step writes the count to
a byte and prints it, adding one, until the count reaches a multiple of 256.
This is why the program prints a long run of strange bytes before every
board. In `simple.c` this is the `reset_count` loop.

7. Positional arguments

`%37$d` means "use argument 37" instead of the next one, so the same argument
can be read or written many times in any order. The `arg` macro passes 73
pointers into one `char d[538]` array, each pointing at a different variable.
`simple.c` gives each of those addresses a name like `p1_won` or `is_tie`.

8. Precision from an argument

`%10$.*37$d` prints 0 with as many digits as the turn counter in argument 37,
which adds the turn counter to the count. Doing it 128 times leaves 128 when
the counter is odd and 0 when it is even, which tells whose turn it is. See
`add_zeros`.

9. Strings built at run time

The board, the `P1>`/`P2 WINS` status and even the `scanf` format `"%hhd"` are
written one character at a time into `d` by `%hhn`. The `scanf` format is
empty in the first round, so the first loop does not wait for input.

10. Side effects hidden in an argument

The 24th argument is `(scanf(d+126,d+4), d+...)`. The comma operator reads the
player's move and then works out which square to mark, all while the
arguments of `printf` are being evaluated.

11. Nested macros

Macros like `T`, `s`, `A`, `n`, `D` and `C` copy their argument 2, 4, 7, 8, 15
and 16 times. Combining them repeats format pieces hundreds of times, turning
a short source file into about 270K of format string.

12. ASCII art layout

The format string is drawn in the shape of a picture, with macro names and
arguments broken across lines and spaces to make it hard to read.
