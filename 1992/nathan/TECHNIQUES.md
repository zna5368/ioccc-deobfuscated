# 1992/nathan

Difficulty: Medium

1. Macros named after library functions

`#define printf sqrt` makes every `printf` in the code mean the variable
`sqrt`, which is really the alphabet table. Neither `printf` nor `sqrt` is ever
called as a function. `#define strnel(sqrt)(sqrt-printf)` looks like a string
function, but it only subtracts the table address from a pointer to get an
index into the table.

2. Swapped and misleading variable names

The parameters of `main` are swapped: `argv` is the argument count and `argc`
is the argument array. Other names are chosen to confuse: `ptr` is an integer
flag for encrypt or decrypt, `cond` is the table length, `watch` is the input
character, `magic` and `define` are pointers into the table, and `i` is the key
pointer. `#define OFST 1` hides the plain number 1 in expressions like
`OFST-1` (0) and `OFST+1` (2).

3. Lookup table split into one-character strings

The 95 printable characters are written as `"1""q""a""z"...`. The compiler
joins neighbouring string literals into one string. The characters are in
keyboard column order (`1qaz2wsx...`) so the table does not look like ASCII.
The table also hides the `-` used for the decrypt option: `i=printf+cond`
points to the end of the table, so `i-1` is the string `"-"`, and `i` itself
is an empty string used as "no key".

4. Whole program inside one `for` statement

All of the setup (skipping the program name, finding the table length,
checking for `-`) is packed into the first part of the `for`, joined with the
comma operator. Restarting the key and reading a character is in the loop
condition, and moving to the next key character is in the increment part. The
loop body is one long expression with no braces.

5. Conditional operator and comma operator instead of `if`

Every decision is made with `?:` and every sequence of steps with `,`. For
example, `ptr?t=watch:0` means "if encrypting, remember the output". Nested
`?:` is used to find the key character: `*i?(define=strchr(printf,*i))?define:
printf:sqrt` means "if there is a key character and it is in the table, use
it, otherwise use index 0". Commas inside function argument lists, such as
`fgetc(((...),stdin))`, make it hard to tell argument separators apart from
the comma operator.

6. Side effects hidden in a function argument

`fgetc(((!*i&&argv>OFST?i=*argc:0),stdin))` restarts the key from the
command line argument when it has run out, as a hidden step before reading the
next input character.

7. Reversed array index

`watch[printf]` is the same as `printf[watch]` in C because `a[b]` means
`*(a+b)`. It picks the output character from the table at index `watch`.

8. One formula for both directions

`(watch-strnel(define))*(ptr*2-OFST)` multiplies the shift by `1` when
encrypting and `-1` when decrypting, so the same expression either adds or
subtracts. `cond*(OFST+1)` adds twice the table length so the value is never
negative before the `% cond`.

9. Variable reused for unrelated values

`watch` first holds the input character, then is overwritten with the previous
cipher index (`watch=t`), and then holds the new output index. `t` is used both
as the "always 0" result of the setup and as the previous cipher index.

10. Disguised as a hello world program

The code is shaped and padded with spaces to look like friendly text, and ends
with the comment `/*world*/`. The extra spaces at the end of short lines also
make an encrypted copy of the source look strange.

11. Old style function definition

`int main(argv,argc)int argv;char*argc[];` uses the pre-ANSI way of declaring
parameter types after the parameter list, which hides the swapped names.
