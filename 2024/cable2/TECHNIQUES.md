# 2024/cable2

Difficulty: Easy

1. Invisible Unicode tag characters in a string literal

The `U"..."` string assigned to `salmon` looks empty except for
`is very yummy`. It is actually full of characters from the Unicode "tag"
block (U+E0000 to U+E007F), which most terminals and editors draw as nothing.
Each tag character is a plain ASCII letter plus 0xE0000, so U+E0053 is a hidden
`S`. Running `cat -v prog.c` shows the raw bytes. The hidden text is the
salmon recipe.

2. Truncation by `putchar()`

`putchar()` takes an `int` but writes only the low 8 bits as an
`unsigned char`. So `putchar(0xE0053)` prints `S`. That is how the 32-bit tag
characters come out as normal ASCII. The end of the hidden text uses the same
trick: U+200A (a "hair space") has a low byte of 0x0A, which prints a newline.
U+2000 (an "en quad" space) has a low byte of 0x00, which prints a NUL byte and
makes `putchar()` return 0.

3. Invisible characters in macro names

The macro names `grill<U+E0041>` and `<U+E0041>` contain or consist of an
invisible tag character (U+E0041 is a hidden `A`). On screen they look like `#define grill ...` and a `#define` with no
name. C accepts these characters in identifiers, so the macros are real and
different from a macro called plain `grill`.

4. Decoy `#undef` lines

`#undef grill` looks like it removes each macro right after it is defined,
which makes the defines look unused. It actually undefines `grill`, a name that
was never defined. The invisible-name macros `grill<U+E0041>` and `<U+E0041>`
stay defined.

5. Hidden code in `main`

`int main() {` is followed by two hidden `<U+E0041>` characters, so the line
looks like a normal opening brace. Those two invisible tokens expand to `grill<U+E0041>`, which expands to
`;while(putchar(*salmon++))`. That gives two loops that print the hidden
string.

6. Decoy loop that never runs

The second injected `while` has no body of its own, so the `for` loop that
prints the squares of 1 to 10 becomes its body. The first `while` stops after
printing the first U+2000 (a NUL). The second `while` prints the next U+2000,
which also returns 0, so the loop ends before its body runs. The squares are
never printed. The visible text `is very yummy` after the two U+2000
characters is never reached either.
