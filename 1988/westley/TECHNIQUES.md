# 1988/westley

Difficulty: Easy

1. Code shaped like a picture

The body of `F_OO()` is laid out as a filled circle made of `_-_-_-_` tokens.
The shape is not decoration: the number of `_` on each line and the number of
lines are the data the program measures. The circle is 16 lines tall and holds
201 `_` marks.

2. Macro that hides a whole statement

`#define _ F-->00||-F-OO--;` means every single `_` in the circle becomes a
full statement ending in `;`. A line like `_-_-_-_` turns into four statements
after the preprocessor runs (`cc -E` shows this).

3. Minus signs that join with the macro

In `_-_-_`, each `-` lands directly in front of the next `_`, so that statement
becomes `-F-->00||-F-OO--;`. The original code (`westley.alt.c`) relied on old
preprocessors gluing `-` and `-a` into `--a`; the modern version instead puts the
extra `-` in front of `F` so it means unary minus. Either way, the first `_` on
a line has no leading `-` and behaves differently from the rest of the line.

4. Short-circuit `||` used as an `if`

`X || Y` only evaluates `Y` when `X` is false. `F` starts at `0` and is
decremented by every `_`, so it is always `0` or negative. For the first `_` on
a line, `F-- > 00` is false, so `-F - OO--` runs and `OO` is decremented. For the
later `_` on the line, `-F-- > 00` is true (minus a negative number is
positive), so `OO--` is skipped. The result is that `F` counts down once per
`_` (the area) and `OO` counts down once per line (the diameter).

5. Expressions evaluated only for their side effects

`-F - OO--` computes a value that is thrown away. Its only purpose is the
`OO--` hidden inside it, while `-F` is noise to make the macro look symmetric.

6. Counting downwards with negative numbers

Both counters end up negative (`F` is `-201`, `OO` is `-16`). The final formula
`4.*-F/OO/OO` negates `F` to get the positive area, and dividing by `OO` twice
cancels the two negative signs, giving `4 * 201 / 256 = 3.141`, the formula
`pi = 4 * area / diameter^2`.

7. Confusing names and octal zeros

The variables are named `F` and `OO` and the function `F_OO`, which look like
the letters around `_` in the circle and like each other. The zeros are written
as `00` (octal zero), which is easily misread as the letter `OO`.
