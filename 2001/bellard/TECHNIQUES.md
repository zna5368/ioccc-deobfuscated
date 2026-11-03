# 2001/bellard

Difficulty: Hard

1. Macros that hide keywords and punctuation

`#define a if(`, `#define i else`, `#define p while(`, `#define J return`,
`#define b ==`, `#define u d!=` and `#define F int` turn the source into a
stream of single letters. `#define f ()` and `#define y (j)` even hide the
parentheses of function calls and definitions, so `o f;` is really `o();`
and `T y{` is the start of the function `T(j){`. Because `n` is `e)`, the
definition `E(n{` is `E(e){` and the call `A(n;` is `A(e);`, which hides
where the argument list ends.

2. Pointer access macros

`#define k *(int*)` and `#define x *(char*)` hide every memory access.
`k d=1;` stores the integer `1` at the address held in `d`, and `x D++=e;`
writes one byte and moves the pointer forward. `#define t +4` makes
`k(d t)` read the second 4-byte field of a symbol table entry.

3. Function call macros

`#define c ad()` makes the token reader look like a plain variable: a line
of `c;c;c;c;` is really four calls that skip four tokens. `#define l ae(`
hides the call that writes machine code, so `l 80);` is "emit byte 0x50".

4. One and two letter names

All 15 globals (`d, z, C, h, P, K, ac, q, G, v, R, D, L, W, M`) and all
functions (`E, o, X, Y, ad, ae, A, s, H, B, S, Z, N, T, O, w, U, I, ab`)
have meaningless names. For example `d` is the current token, `h` the
current character, `q` the place where the next instruction is written and
`v` the next free byte of global data.

5. Pointers stored in `int`

Every pointer (code buffer, data buffer, symbol table, file text) is kept in
an `int` and turned back into a pointer with a cast only when it is used.
This also means the symbol "value" of a name can be a stack offset, a code
address, a data address or the special numbers `0` (not defined yet) and `1`
(a `#define`), all in the same slot. Addresses below 512 are treated as
offsets from the stack frame (`(e<512)<<7|5`).

6. Machine code written as decimal numbers

The compiler writes i386 instructions as integers in decimal, and `ae()`
outputs the non-zero bytes lowest first. `15042901` is `0xE58955`
(`push %ebp; mov %esp,%ebp`), `50121` is `0xC3C9` (`leave; ret`),
`60545` is `0xEC81` (`sub $n,%esp`), `1032325` is `0x0FC085`
(`test %eax,%eax` and the first byte of a conditional jump), and small ones
like `80`, `89`, `184` and `233` are `push %eax`, `pop %ecx`, `mov $n,%eax`
and `jmp`.

7. Packed operator table string

`"++#m--%am*@R<^1c/@%[_[H3c..."` encodes every operator. Each entry is the
first character, the second character (`@` means none), then digits in base
64 (characters below `b`) that build the machine code for the operator, ended
by one letter `b`..`l` whose distance from `b` is the operator's precedence
level. For example `+@.B#d` is `+`, alone, code `0xC801` (`add %ecx,%eax`),
level 2.

8. Keyword numbers from string offsets

Keywords are found with `strstr` in the string
`" int if else while break return for define main "`, and the token number is
`offset * 8 + 256`. So the code tests magic numbers such as `256` (`int`),
`288` (`if`), `312` (`else`), `352` (`while`), `400` (`break`),
`448` (`return`), `504` (`for`), `536` (`define`) and `592` (`main`). For
user names that number is then added to the symbol table base, so the token
itself becomes the address of that name's symbol slot.

9. Character and number constants written as numbers

Characters are compared with their ASCII codes: `95` is `'_'`, `92` is
`'\\'`, `110` is `'n'`, `35` is `'#'`, `39` is `'\''`, `47` and `42` are
`'/'` and `'*'`, and `34`, `40`, `41`, `44`, `59`, `61`, `123`, `125` are
`"`, `(`, `)`, `,`, `;`, `=`, `{`, `}`. Small special values like `2`
(number token and end of macro marker) are also left unexplained.

10. Reused variables and side effects in expressions

Variables change meaning half way through a function: in `T()` the
parameter `j` first means "assignment allowed" and then becomes the stack
offset of the call arguments, and `e` holds the token and later the cast
type. In `I()`, `e` is the keyword, then a jump address. Assignments are
hidden in calls, for example `r S(e,m);` (`m=S(e,m)`) and
`k r j;` (`*(int*)m = j`), and `#define r m=` hides the assignment entirely.

11. Short-circuit jump chains stored inside the code

Jumps whose target is not known yet are linked together through their own
4-byte address field: each one holds the address of the previous one, and
`A()` walks the chain and fills in the real offsets. The same trick links
calls to functions that are used before they are defined, through the
second field of the symbol slot (`k(d t)`).

12. No prototypes and K&R style

Functions have no return types or prototypes, are called before they are
defined (`w f` is used in `T` before `w` is written), and the parameters
have no types, so every value is an implicit `int`.
