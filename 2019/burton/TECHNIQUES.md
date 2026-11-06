# 2019/burton

Difficulty: Medium

1. Meaningless single-letter names

The variables `e`, `n`, `j`, `o`, `y` spell "enjoy" but say nothing about
their jobs. `e` is the line count, `j` the word count, `y` the character
count, `n` the current character and `o` the "am I outside a word?" state.

2. Implicit `int` globals and implicit declarations

`e,n,j,o,y;` declares five global variables with no type, and `main()` has no
return type. Old C assumes `int` when the type is missing. `getchar` and
`printf` are used without `#include <stdio.h>`, so the compiler guesses their
declarations. Globals also start at zero, so none of the counters need to be
set to zero by hand.

3. Bitwise trick to shift `getchar()`

`n=-~getchar()` uses `-~x`, which equals `x + 1` in two's complement. `EOF` is
`-1`, so `n` becomes `0` at end of file and the `for` loop stops on its own,
without comparing to `EOF`. Because every character is shifted up by one, all
later comparisons use shifted values: `11==n` really means "the character is
`'\n'` (10)".

4. Disguised numeric and character constants

The ranges and characters are written in mixed bases and as XOR results to
hide what they are: `0xe` is 14, `012` is octal 10, and `'`'^n^65` is zero
only when `n` is `'`' ^ 65` = 33, i.e. the character is a space (32 + 1).
`n>0xe ^ 012>n` is true when the character is above `'\r'` or below `'\t'`,
which is "not one of the control whitespace characters `\t \n \v \f \r`".

5. XOR used as logical OR and as "not equal"

`n>0xe ^ 012>n` uses bitwise `^` on two comparisons. Both cannot be true at
the same time, so `^` acts like `||`. In `'`'^n^65` the XOR gives zero only
when the two values are equal, so it acts like `!=`.

6. Operator precedence abuse

The expression `n>0xe^012>n&&'`'^n^65?!n:!o?++j:o` has no parentheses. The
reader must know that `>` binds tighter than `^`, `^` tighter than `&&`, and
`&&` tighter than `?:` to see that it means
`((n>14) ^ (10>n)) && (n ^ 33) ? ... : ...`. The original Makefile even turns
off `-Wparentheses` to hide the warning.

7. Nested ternary instead of `if` statements

`o = <not space> ? !n : !o ? ++j : o` packs the whole word-counting state
machine into one assignment. For a non-space, `!n` is always `0`, meaning
"inside a word". For a space, if we were inside a word (`o == 0`) the word
count is bumped with `++j` and that new non-zero value is stored in `o`,
meaning "outside a word". Otherwise `o` stays unchanged. The state flag and
the counter are mixed together.

8. Loop body hidden in the `for` header

All the work happens in the `for` statement: `++o` in the initializer sets the
start state to "outside a word", the condition reads the next character, and
the increment part `e+=11==n,y++` uses the comma operator to count newlines
(adding a comparison result of 0 or 1) and characters at the same time.

9. Leftover values reused in the final `printf`

`e^n` prints the line count because `n` is always `0` after the loop ends, so
XOR with it changes nothing. `j+=!o&&y` adds the final word only if the input
ended in the middle of a word (`o == 0`); this happens inside the argument
list instead of before the call.

10. One-line layout with no whitespace

The whole program is a single 127 character line with no spaces, so there is
no indentation or structure to guide the reader.
