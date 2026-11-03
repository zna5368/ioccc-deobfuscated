# 2000/dlowe

Difficulty: Hard

1. A second language hidden inside a C string

Most of the program is not C at all. The big string passed to
`perl_eval_sv(newSVpv("_:$_= <> ; ...", 0), 0)` is a Perl script, and C only
starts a Perl interpreter (`perl_alloc`, `perl_construct`, `perl_parse`) and
hands it that string. Reading the input, parsing numbers, keeping the stack and
printing are all done in Perl. A reader who only knows C sees a string full of
`$`, `@` and `\\` and has no idea it is the main program. simple.c keeps the
embedded script but writes it one statement per line with comments.

2. C functions called back from Perl (XSUBs)

The math operators are C functions that Perl calls like normal Perl subs. The
macro `XS(name)` declares a function with the special signature Perl expects,
and `newXSproto("UU", ff, qq, "")` inside `bb()` registers the C function `ff`
under the Perl name `UU`. `bb` is passed to `perl_parse`, which runs it while
starting up. The C functions then reach into Perl's memory with
`perl_get_av("SS", 0)` (the Perl array `@SS`), `av_pop`, `av_push`, `SvNV`
(read a value as a double) and `newSVnv` (make a new Perl number). Because these
calls are poorly documented and mostly macros, the connection between the two
languages is very hard to follow. simple.c names the subs `add`, `subtract`,
and so on, and names the stack `@stack`.

3. One macro generates six functions

`ZZ(YY, WW)` expands to a complete XS function: check the stack has two values,
pop them, check for division by zero, then push the result of the expression
`WW`. The six lines `ZZ(uu, ...)`, `ZZ(ii, ...)` ... produce modulus, subtract,
add, power, divide and multiply. Inside `WW` the names `uu` and `Ss` refer to
the variables the macro declares (the left and right operands), so the
expression only makes sense after mentally expanding the macro. simple.c has a
single `apply_binary_operation()` helper and one small named function per
operator.

4. Command dispatch by computing a function name

There is no `switch` on the command letter. Instead the Perl code runs
`&{chr(((ord)%39)+3**4)x2}`: it takes the character code of the token, keeps
the remainder after dividing by 39, adds 81 (`3**4`), turns it back into a
character and repeats it twice to build a sub name. For example `p` is 112,
112 % 39 = 34, 34 + 81 = 115 = `s`, so the sub `ss` (print top) is called;
`+` becomes `UU`, which is the C add function. Because many characters share
the same remainder, every command has synonyms (`=` works like `d`). simple.c
builds a table that maps each remainder to a clearly named sub.

5. AUTOLOAD used as the error handler

When the computed name does not exist (for example `bb` or `QQ`), Perl would
normally die. The script defines `sub AUTOLOAD`, a special Perl sub that runs
whenever an unknown sub is called, and makes it print "unimplemented". The
error case is never written as a test. simple.c checks whether the table has an
entry and prints the message in an explicit `else`.

6. Meaningless and reused short names

All identifiers are doubled letters: `ff`, `ii`, `uu`, `gg`, `ss`, `Ss`, `SS`,
`aa`, `bb`, `UU`, `WW`, `TT`, `YY`, `vv`, `__`, `_`. The same name means
different things in different places: `ff` is the C add function but also the
Perl sub that clears the stack, `ii` is a C local variable, a C function
(subtract) and the Perl sub that prints the stack, and `uu` is both a C function
and a local variable inside it. `_` is the argument count of `main` in C and the
"divide by zero" sub and a `goto` label in Perl. simple.c gives everything a
descriptive name.

7. goto and ?: instead of if and loops

Neither language uses `if`, `while` or `for`. The Perl main loop is two labels,
`_:` (read a line) and `__:` (take the next word), joined by `goto _` and
`goto __`, and decisions are written as `(test) ? (a) : (b)` or with `and` /
`or` (`defined or exit`). The C macro body is one huge expression built from
`?:` and commas. simple.c uses ordinary `if`/`else`, `while` and `foreach`.

8. goto &NAME to report errors

Perl subs such as `ss` begin with `$#SS<0 and goto &__;`. `$#SS` is the last
index of the array (so `< 0` means empty), and `goto &__` replaces the current
sub with a call to `__`, which prints "stack empty". It is really just
"if the stack is empty, report it and return". simple.c writes
`return stack_empty() if !@stack;`.

9. Arithmetic swap

The `r` command swaps the top two values without a temporary variable:
`$SS[$#SS]+=$SS[$#SS-1]; $SS[$#SS-1]=$SS[$#SS]-$SS[$#SS-1]; $SS[$#SS]-=$SS[$#SS-1]`.
Adding the two values and subtracting each back out leaves them exchanged. It
is hard to recognise, and it also turns the values into numbers (so `2.50`
becomes `2.5`). simple.c keeps the arithmetic to give identical output but
names the intermediate `$sum` and explains it.

10. Abuse of Perl's default variable and the argument list

The script reads lines into `$_` with `_:$_=<>;`, then overwrites `@ARGV`
(normally the command line arguments) with the words of the line
(`@ARGV=split`) and pulls them out one by one with a bare `shift`, which
silently means `shift @ARGV` here. `split`, `chomp`, `ord` and the regex matches
all work on `$_` without naming it. The C code depends on this too: it reads
`perl_get_sv("_", 0)` to find out which command is running. simple.c uses
named variables `$line` and `$token`, and the C code reads `$token`.

11. Hidden division-by-zero test using magic numbers

Each C operator computes `ii = first character of $_ % (36+cc)` and compares it
with `pow(2,cc)` and `40-cc`. With `cc` defined as `(((3)))` these are `% 39`,
`8` and `37`, which are the remainders of `/` (47) and `%` (37). So only the
divide and modulus commands check for a zero divisor, but the numbers do not
show that. Other constants are dressed up too: `cc-cc`, `0*cc` and `0/cc` are
all `0`, and `3-cc` is `0`. simple.c writes `'/' % 39` and `'%' % 39`.

12. Escaped characters inside the Perl regexes and strings

The regex `m*^\\x70oO\\x74$*i` uses `*` as the regex delimiter instead of `/`,
and hex escapes so that it reads as "pOot" only after decoding (`\x70` is `p`,
`\x74` is `t`). The same trick is in `"$_ w\\x61s h\\145r\\x65!"` which is
`"$_ was here!"` (`\145` is octal for `e`). The double backslashes are needed
because the Perl code is inside a C string. This is the hidden easter egg: the
command `poot` (any capitalisation) prints "poot was here!".

13. Shape of the source code

The code is arranged as three columns of text with large gaps, and strings and
macro arguments are split across lines at odd places (`"W""W"`, `"Y" "Y"`,
`"AUTOLO" "AD"`, a `\` line continuation in the middle of `perl_\ parse`). C
joins adjacent string literals together, so the pieces still form the right
names, but the layout hides where statements begin and end.
