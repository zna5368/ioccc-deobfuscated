# 2000/primenum

Difficulty: Easy

1. Misdirection: pseudocode for the wrong algorithm

The program is written to look like a textbook prime sieve: `ForLoop (PossiblePrime SetTo SmallestPrime ; ... Increment PossiblePrime)` with an inner loop over `PossibleFactor` and a test `PossiblePrime Modulo PossibleFactor Is Zero`. Together with the file name `primenum.c`, this makes the reader assume they already know what the program does, so they do not check what each word really expands to. The actual program reads text and scrambles it; no prime is ever computed.

2. Macros that lie about their meaning

Every word in the program body is a `#define`, and many of them do not mean what their names say. `LessThanOrEqualTo` is `!=`, `LessThan` is `*(c&64)*` (a multiplication, not a comparison), `CommandLineArgument` is `-1` (the value of EOF), `Zero` is `i%j` (not 0), and `PrimeNumber` is `(c^(!i*n%64));`, the XOR that scrambles a character. To see the real code, run the file through the preprocessor (`cpp`) and read the result.

3. Look-alike identifiers (`l` vs `1`)

`PossiblePrime` and `Possib1ePrime` differ only by a lowercase `l` and the digit `1`. The first becomes the loop counter `i`, and the second becomes `(c=getchar())`, which reads the next input character. In the same way `Flag` and `F1ag` are two different variables. The author warns about this with "not a11 variab1es1es are identica1".

4. Macro that renames a variable, including in its declaration

`#define i F1ag` turns every `i` into `F1ag`, and because macros are expanded again after substitution, this includes the `i` inside `Declare` (`int i,j,n,Flag=1;`) and inside `Zero`. So the variable that is declared is really named `F1ag`, and the `Flag` that is tested later is a different variable that stays at 1 forever.

5. Useless variables and a useless `if`

`Flag` is set to 1 and never changed, so `If (Flag Is True)` is always true and the character is always printed. `argc` is never used, and the outer loop's `++F1ag` has no effect because `F1ag` is overwritten with 1 at the start of each pass.

6. A `for` loop used as an `if`

The inner loop is `for (j = 2; j * (c & 64) * F1ag; ++j) if (F1ag % j == F1ag % j) F1ag = 0;`. The comparison of a value with itself is always true, so the first pass sets `F1ag` to 0, and then the loop condition becomes 0 and the loop ends. The loop body runs at most once: only when the character has bit 64 set. The whole loop is a disguised `F1ag = (c & 64) ? 0 : 1;`.

7. Hidden XOR cipher

The output is `c ^ (!F1ag * n % 64)`. Because `!F1ag` is 1 only when the character has bit 64 set, this XORs letters (and other characters from `@` upward) with the key `n % 64` and leaves the rest alone. A key below 64 can never change bit 64, so the same characters get XORed again on a second run, which undoes the scrambling. With key 32 this just flips upper and lower case.

8. Reliance on `char` and the high bit

The input is stored in a `char` and compared with `-1` (EOF). With a signed `char`, a byte value of 255 also looks like -1 and stops the program early, so the program depends on the high bit of the input being 0. The original also uses `atoi`, `getchar` and `putchar` with no `#include`, relying on old implicit function declarations.
