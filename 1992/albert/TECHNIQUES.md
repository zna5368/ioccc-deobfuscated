# 1992/albert

Difficulty: Insane

1. setjmp/longjmp used as loops

`ppPppp()` and `aaAaaa()` call `setjmp()` on a `jmp_buf` stored inside each
list node, then jump back to it with `longjmp()`. The value passed decides
which branch runs: `1` jumps into the `else` part (end of a conversion pass),
`2` jumps back into the main part (start a new pass). The lowest digit's
frame therefore acts as the head of an endless loop. When a factor is found,
the jump goes to a different node's `jmp_buf`, so the loop head silently moves
to another stack frame.

2. Recursion used to walk a list

Neither function uses a loop to visit the digits. `aaAaaa()` calls itself once
per input character to build the list, and `ppPppp()` calls itself once per
digit so that, as the calls return, each digit is processed from the top down.
The way out of the recursion is a `longjmp()` rather than a `return`.

3. Two algorithms merged into one

The conversion from base `b+1` down to `b` (adding the next digit, with a
carry) and from base `b-1` up to `b` (subtracting the next digit, with a
borrow) share one `if` condition:
`qqq<47 && (qq=0,p->qq+=p->p->qq-96)>=48-qqq || qqq>46 && (p->qq-=p->p->qq)<0`.
The flag `qq` (0 or 4) chooses between `+= 1` and `-= 1` for the carry and
also fixes the offset added to the digit. The parsing function `aaAaaa()`
likewise both reads the number and validates it, and its error message
doubles as a way to restore the program name.

4. One counter hiding several meanings

`qqq` starts at 39 and counts up. While below 47, `48-qqq` is the base being
converted to (9 down to 2); from 47 on, `qqq-44` is the next base and
`qqq-45` is the factor printed. `qqq -= 1` followed later by `qqq += 1` in the
`longjmp()` target cancels out.

5. Digits stored as ASCII characters

Each digit is kept as its character code (`'0'` is 48), so the code is full of
`48` and `96` (two times 48) to add, subtract and compare digits.

6. Division by rotating a circular list

Dividing by the base is done by moving the global "top" pointer onto the
trailing zero (`*pp = ppp`). In a circular list this turns the last digit
into a leading zero, which is later removed by the leading zero clean-up
loop. No division instruction appears anywhere.

7. Confusing names

Every identifier is made of `a`, `p` or `q`: `aaaaaa` is 2, `aaaaaaa` is 1,
`P` is both the global top pointer and the "previous" field, `p` is the
"next" field, `qq` is the digit and also a local flag, and `qqq` is the
counter. Function names `aaAaaa` and `ppPppp` follow the same pattern.

8. Strange string trimming and error message

`*aa=0; aa+=strlen(aa+1);` saves and blanks the first character so the
recursion stops there, then points at the last character. The usage message
prints `(aaa[0]?7:aaaa)` and `aaa+!aaa[0]`: normally a bell character (7) and
the program name, but when the number is the program name itself it prints
the saved first character followed by the rest of the name.

9. Extra parentheses

Conditions are wrapped in five layers of parentheses, such as
`if ((((( !setjmp(P->ppp) )))))`, which changes nothing but makes the
lines harder to read.

10. K&R style declarations and a malloc macro

Functions use old style parameter lists and implicit `int` returns, and
`new(PP)` is a macro for `(PP *) malloc(sizeof(PP))`.
