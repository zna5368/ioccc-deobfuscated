# 1986/hague

Difficulty: Medium

1. Morse code macros

The source starts with `#define DIT (`, `#define DAH )`, `#define __DAH ++`,
`#define DITDAH *`, `#define DAHDIT for`, `#define DIT_DAH malloc`,
`#define DAH_DIT fgets` and `#define _DAHDIT char`. Every bracket, `*`, `++`,
`for` and type name is replaced by a word made of "DIT" and "DAH", so the
program reads like Morse code instead of C. Running the preprocessor (or
replacing each macro by hand) turns it back into normal C.

2. Look-alike identifiers

The variables and functions are all named with the same pieces: `_DIT`, `DIT_`,
`_DIT_`, `DAH_`, `_DAH`, `__DIT` and `_DAH_`. Only the placement of the
underscores tells them apart, and they also look like the macros above, so it is
very hard to remember which name is the line buffer, which is the current
character, which is the table pointer, and which is a function. In simple.c
they are `line`, `p`, `entry`, `morse_tree`, `print_morse` and `put_char`.

3. Morse code tree packed into a string

`_DAH_[]="ETIANMSURWDKGOHVFaLaPJBXCYZQb54a3d2f16g7c8a90l?e'b.s;i,d:"` is a
binary tree of Morse code stored in "heap order": the character at position
`n` (counting from 2) has the code whose binary digits after the leading 1 are
the dots (0) and dashes (1). For example E is 2 = `10` = `.` and A is 5 =
`101` = `.-`. Positions with no symbol are not stored; a lowercase letter
stands for a gap and tells the program to skip `letter - 'a'` extra
positions (`*DIT_ += (*_DIT_ >= 'a' ? *_DIT_ - 'a' : 0)`). This keeps the table
short but hides what it is.

4. Hidden storage in the input buffer

`DIT_ = _DIT++` keeps the first byte of the `malloc(81)` buffer as the variable
holding the current Morse code number and then moves the line pointer one byte
forward. The code number therefore lives in a `char` inside the text buffer
instead of in a normal variable. simple.c uses a plain `int code`.

5. All work packed into `for` loop headers

`main` is three nested `for` loops with almost empty bodies. Reading a line,
removing the newline, printing a newline, printing each code, printing `?` for
unknown characters and printing the space separator are all done in the
initialization and increment parts of the loops, joined by the comma operator.
The search loop's condition also does the uppercase conversion. simple.c moves
these into ordinary statements inside the loop bodies.

6. Bit tricks

`*DAH_ & 223` clears bit 5 (value 32) to change a lowercase letter into
uppercase, and `DIT_ & 1 ? '-' : '.'` with `DIT_ >> 1` reads the code number one
binary digit at a time. These are short but not obvious ways to write "to
uppercase" and "next dot or dash".

7. Recursion that prints through its return value

`_DAH` calls `__DIT(DIT_ > 3 ? _DAH(DIT_ >> 1) : '\0')` and then returns the
last dot or dash, which the caller prints. Each call prints the symbol of the
level above it, so the output comes out in the correct order but the printing
is split between the function and its caller. When the recursion reaches the
top it prints a `'\0'` byte, so every Morse symbol is preceded by an invisible
NUL character. simple.c does the printing inside `print_morse` and keeps the NUL.

8. Old style declarations and implicit int

`main`, `_DAH` and `__DIT` have no return type, `malloc()` and `fgets()` are
declared inside `main` as returning `char *`, and `__DIT` uses the old K&R
parameter style `__DIT(DIT_) char DIT_;`. Beginners rarely see these forms, so
it is unclear what the types are.

9. Shaped source

The code is laid out in tab-separated columns so it looks like rows of Morse
code dots and dashes, which hides the structure of the statements.
