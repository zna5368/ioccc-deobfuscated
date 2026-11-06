# 2020/ferguson2

Difficulty: Hard

1. Digraphs and `%:` for `#`

The code writes `<:` and `:>` for `[` and `]`, `<%` and `%>` for `{` and `}`,
and `%:include` and `%:define` for `#include` and `#define`. They mean exactly
the same thing, but they are hard to read and confuse code formatters. For
example `Y<:7:><:1:>` is `Y[7][1]`.

2. Prompt strings enciphered by the program itself

The prompts `Ring`, `Setting`, `Position`, `Plugboard pair` and `Reflector` are
stored scrambled (`"Mjqq"`, `"Lcxvago"`, `"Huwfnbgt"`, `"Hdoqhmket fbvy"`,
`"Mcromlhxo"`). At the start `main` runs the Enigma function `m()` on each of
them with the default settings, which turns them back into readable words.
Anyone reading the source sees no hint that this is an Enigma machine.
`simple.c` stores the plain words.

3. Many arrays packed into one big array

`char Y[20][27]` holds everything: the five rotor wirings (`Y[0]` to `Y[4]`),
the two reflectors (`Y[5]`, `Y[6]`), the settings (`Y[7]`), the alphabet
(`Y[8]`), the five prompts (`Y[9]` to `Y[13]`), the two plugboard columns
(`Y[14]`, `Y[15]`) and the notches of the rotors in use (only the first
character of `Y[16]`, `Y[17]` and `Y[18]`). So a reader must know which row is
which. The reflector is picked with `Y[e[-10]+5]`. `simple.c` gives each one a
named array.

4. Interleaved settings in one string

`Y[7]` is `"ABBBCCQEVJZ"`. Ring settings sit at the even places 0, 2 and 4,
start positions at the odd places 1, 3 and 5, and the rotor notches `QEVJZ`
come after that. The program reaches them with sums like `i+i%3` (which is
`2*i` for 0, 1, 2) and `i+i+1`, and the macro `A(y)` is `*(&Y[7][1]+(y))`,
which is just `Y[7][1+y]`. `simple.c` uses the arrays `ring_setting`,
`start_position` and `rotor_notch`.

5. One int array used for unrelated values, with negative indexes

`z` is a compound literal `(int[13]){0}` and `e` points to `z + 10`. Then
`e[-10]` is the reflector, `e[-6]`, `e[-5]`, `e[-4]` are rotor offsets,
`e[-3]` to `e[-1]` are the chosen rotors, `e[1]` is a loop counter and `e[2]`
is another loop counter. `g` is `e + 1` and `P` is `e + 2`, so `*g`, `e[1]`,
`*(e+1)` and `1[e]` are all the same variable. The rotor list is filled with
`for (j=-3; j<0; ++j) e[j] = j+3;`, a loop over negative indexes. `simple.c`
uses ordinary named variables and local loop counters.

6. Scratch characters hidden in a string

`S` points to `Y[7]+6` (the notches) and `r` points to `S+10`. `r[-1]`,
`r[-2]` and `r[-3]` are unused bytes at the end of `Y[7]` that the plugboard
function uses as temporary variables. The plugboard function `Z(x)` is called
as `Z(r[-1]=k)` so the letter is stored in `r[-1]` and the parameter `x` is
almost never used. `simple.c` passes the letter as a normal parameter.

7. Misleading function parameters

`w(char *s, int k, int v)` is passed the loop counter `*g` as `v` but only
uses `v` as a temporary. Its loop counts with `e[2]` but checks `s[*P]`, which
is the same thing written another way. In `Q()` the call
`w(R[*g], *(e+e[1]-6)=q(Y[8],*Y[7]), *g)` also hides an assignment inside the
argument list.

8. Macros that hide common code

`D` is `strchr(`, `q(s,x)` finds the place of `x` in `s`, `V(k)` is "find the
place of `k` in the alphabet", `W(a,b)` is "the next letter after `b`",
`d(a,b)` and `O(a)` do half of a rotor step each, `J(x)` reads a character
that is not a newline, `K` is `fprintf(stderr,` and `l(c)` turns `c` into a
capital letter while also assigning it. `simple.c` uses small functions such
as `letter_index`, `next_letter`, `rotor_forward`, `rotor_backward` and
`read_char`.

9. `&&` in place of `if`

Statements such as `c == *Y[18] && (f = 1);` and
`(Y[e[-10]+5][k-'A']) && (k = Y[e[-10]+5][k-'A']);` use the fact that the
right side of `&&` only runs when the left side is true. In
`f && (f = 0 && (a = W(Y[8],a)));` the `0 &&` means the left rotor is never
stepped in that case. `simple.c` uses `if` statements.

10. Rotor passes written out in one long line

The six rotor passes (three forward, three back) are written as a long chain
of `V(k); d(...); V(v); O(...);` with the rotor offsets in `e[-6]` to `e[-4]`.
`simple.c` writes each pass as a call to `rotor_forward` or `rotor_backward`.

11. Odd number tricks

`v > 64 && v < 91` checks for a capital letter (`'A'` is 65, `'Z'` is 90).
`for (j += 9; ...)` reuses the 0 left from the previous loop to start at 9.
`(j < 1 ? 1 : (j > 1 ? 2 : j)) - 1` looks like a choice of three values but
gives 1 (reflector C) for any normal character. `14[Y]` and `15[Y]` mean
`Y[14]` and `Y[15]`.

12. `goto` and one-letter names

`main` uses `goto k` to jump over reading the settings. All names are single
letters, and `E` is both a function and a local variable in `m()`. `simple.c`
uses an `if` and names like `encipher`, `plugboard`, `setup_rotors` and
`read_settings`.
