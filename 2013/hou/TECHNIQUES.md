# 2013/hou

Difficulty: Terrifying

1. Compressed source code

`hou.c` is not the real program. The string `s` is a compressed copy of another
C file, and all `main` does is unpack it and print it with `putchar`. To guess
each next character, the loop `while(++v<i)` looks back through everything
printed so far for places where the same characters came before, and counts
what followed them. Longer matches count for more. This is called prediction by
partial matching. An arithmetic decoder then uses those counts to turn the
numbers in `s` into characters, so a likely character costs only a fraction of a
byte. None of the real code can be read until the program has been run.
`simple.c` holds the unpacked program directly.

2. Two stage build

The Makefile compiles `hou.c`, runs it, and pipes what it prints straight back
into the compiler with `./hou | cc -x c - -o hou`. The second compile overwrites
the first program with a different one. The unpacked source is never saved to a
file, so a reader looking in the directory finds only the unpacker.

3. Data packed around the size counter

The contest's size tool does not count whitespace, so the compressed data is
written to use it. `s` is read two characters at a time as one digit of a base
376 number: `x*v-32-(*s+24)/58+s[1]%6%4*94+*s`. The first character gives a value
up to 92. The second gives only two bits through `%6%4`, where a space gives 2
and a tab gives 3, so about half of the second characters are free whitespace.
The `(*s+24)/58` part closes the gaps left by never using `"` and `\`, which
would need escaping inside a C string.

4. Model settings hidden in the output buffer

`l` starts with the bytes `" \2\6\t\f\f\f "`, which look like stray whitespace.
They are the weights for each match length, read by `w=l[j++]` and squared by
`w*=w`. The decoded text is then added to the same array by `l[i++]=j`, starting
at position 9, so one array is both the settings and the output. Positions also
hold `j` rather than the character itself, and `putchar(j?j+32:10)` converts
back. Symbol 0 is a newline, which is why the unpacked file uses newlines where
a space is required.

5. Program written in a second language

Most of the rendering is not in C. The string `A` holds scripts such as
`5]R=0;x=1;y=1;z=1;#=0-1;#=7:T;X=x*X;Y=y*Y;Z=z*Z`, which the C code interprets
while it runs. Reading the C alone does not show what the camera or any surface
does. `simple.c` splits the string into one piece per script, with a comment
above each piece.

6. Numbers that are really variables

The script language has no numbers. In `1/2`, the `1` and the `2` are variables.
The loop `for(G=A;*G++;Y[x=*G]=*G-48)` gives every character in the script the
value of its ASCII code minus 48, so `2` happens to hold 2 and `p` holds 64. This
is why `G=p*:` means 64 times 10, and why `d=4*;;` has a `;` that is a variable
holding 11, followed by another `;` that really ends the statement.

7. Operators that do not follow normal rules

The operator list `":#<-+/*"` gives the script its own order of operations. `*`
is done before `/`, and `+` before `-`, and every operator groups from the right
because `P` calls itself for its right side. So `a/b*c` means `a/(b*c)` and
`a-b-c` means `a-(b-c)`, which look like ordinary maths but give different
answers. `#` is a choice that reads a hidden third input, the variable named `#`.

8. Program and data in the same string

The first 224 bytes of `A` are not script at all. They are a table of boxes with
every number stored as a printable character, plus 62. The script reads its own
text as data using the `0:U` built in, and moves `U` along one byte at a time, so
`@|?|?z#` is a box and not garbage. The run also starts by testing `G[2]==61` on
those bytes, relying on the table not looking like a statement.

9. Nested conditionals in place of a switch

`P` picks an operator with `*++C?*++C?*++C?*++C?*++C?*++C?`, counting how many
characters are left in the operator list, and then lists all seven results
backwards at the end of the line. Built in functions are chosen the same way with
seven copies of `z-->0?`, counting the left operand down to zero. The answer for
each case is far from the test that selects it. `simple.c` uses a `switch` and a
list of `if` statements.

10. One array for every kind of state

`Y` holds script variables, ray state, random numbers, a saved copy of the last
path, statistics and the image itself, all reached with bare numbers. `Y[82]` is
the script variable `R` because 82 is the ASCII code for `R`, and `Y[286]` is
`Y[32]` in the saved copy, because the copy starts at 254. `Y[333]` belongs to the
saved copy too, but it is borrowed to hold an unrelated probability. `simple.c`
gives each position a name in an `enum`.

11. Short names reused for unrelated jobs

Every name is one letter, and many have several jobs. `X` starts as the argument
count of `main` and then becomes the axis a ray is moving along. `C` is a loop
counter, the variable being assigned to, and the size of a grid step. `w` is a
script number while the grid is built and a random number position later. `v` is
both a global random seed and the `argv` of `main`, which hides the global inside
`main`. `P`, `H` and `J` also share letters with script variables that mean
something else. `simple.c` gives each job its own variable.

12. Values left behind in a scratch variable

`Y[7]` is written by `P`, `H` and `main` as a temporary. Later code quietly reads
what was left there: `Y[85+x]+=Y[7]*Y[88+x]` moves the ray by the last value
stored, even on a turn where no ray was walked. That is harmless only because
nothing uses the moved position on those turns, which the code does not show. The
reader has to work out by hand which write each read sees.

13. Infinity and not-a-number used on purpose

`Y[3+x]=1/Y[88+x]` divides by zero when a ray has no movement on an axis. The
result is infinity, which then loses the "which border is nearest" test, so no
special case is written. `Y[333]=Y[32]/Y[286]` divides by zero on the first
path, and `Y[333]<1?Y[333]:1` turns the infinity or not-a-number into 1. The code
looks like a bug and only works because of the IEEE 754 rules for `double`.

14. Bit tricks with masks

`D>>3&07770707` is an octal mask. Each octal digit is three bits, so the zeros
clear the low half of each coordinate in a single step, turning a voxel number
into the number of its 8x8x8 block. `r[X]&=-C` rounds down to a multiple of `C`,
because `-C` has zeros in the low bits when `C` is a power of two.
`r[3+X]&(C-1)` turns a direction of plus or minus one into a small correction.

15. Assignments and commas inside expressions

Work is hidden inside other statements. `fprintf(W=fopen(J[93]+1,"wb"),...,
C=Y[71],x=Y[72],Z)` opens a file and sets two variables inside the argument list.
`Y[D=512+Y[263]*3+x]+=...` assigns while computing an index, only to convert a
`double` to an `int`. Long comma chains such as the one after `for(x=0;x<3;x++)`
put several steps into one statement, so there are no braces showing the shape.

16. Tables with more than one meaning

`J` is the list of script labels, but `J[0]` is a `printf` format that is replaced
after the first image, and `J[1]` collects every character that is not a label.
`J[93]` points into the script at the second `]` of `]]luna.ppm`. That `]` is
overwritten by `*J[93]=X+49`, so `J[93]` is the session file name and `J[93]+1`
is the image file name, from the same text.

17. ASCII codes in place of characters

Characters are written as numbers: `G[2]==61` tests for `=`, `*G-93` tests for `]`
and `J[x=Y[82]+48]` converts a script number to a label. Nothing in the code says
that these are characters at all.

18. Loop counters stored as floating point script variables

`for(Y[88]=0;Y[88]<64;Y[88]++)` and `for(Y[80]=-1;Y[80]<5;Y[80]++)` keep their
counters in `Y` so that scripts can see them as `X` and `P`. A script can change
them too, and `P=9` in a light source ends a loop in the C code from inside the
script, so the reason the loop stops cannot be found by reading the C.
