# 1991/brnstnd

Difficulty: Hard

1. Single letter macro alphabet

Almost every letter is a `#define`: `f` is `if`, `Y` is `while`, `v` is
`return`, `b` is `break;`, `h` is `;continue;`, `R` is `char*`, `L` is
`strlen`, `w` is `[256]`, and even `D` is `define`. The code reads like
random letters until every macro is expanded in your head. The fix is to run
the preprocessor and replace each macro with the C it stands for.

2. Character pool lookup for command letters

Most commands are compared against characters picked out of the string
`I="I'd love ta win: the most useful !$>%`/#<&*|^ _OBFUSCATED_ utility. Pleez?"`
by position, for example `J(27,...)` means "if the character is `I[27]`",
which is `s`, and `J(39,...)` is `#`. The error message `?` is printed with
`puts(73+I)`, which is just the last character of that sentence. You have to
count characters in the sentence to know which command each block handles.
In simple.c every command is a plain `case 'x':` in a `switch`.

3. Macros with unbalanced brackets

Several macros end or start in the middle of an array index. `S` is `i[q`,
`_` is `s[p`, `e` is `_]` (so `s[p]`), `d` is `i[++q]`, `N` is `s[++p]` and
`u` is `_--]` (so `s[p--]`). So `y+1]=z[c]` inside `H(x,y,z)` becomes
`s[p+1]=s[c]` only after the brackets are glued together. The brackets look
unmatched in the source, which hides what is being indexed.

4. Misleading variable names

The author points out that `i` is the string stack and `s` is the integer
stack, the opposite of what the letters suggest. `p` and `q` are the tops of
those stacks, `Z` holds the stored programs, `K` and `k` are a buffer of
pending characters, `a` and `A` are `argc` and `argv`, and `U` and `T` are
reused both as function names and as local variable names inside the `O`
macro. simple.c renames all of these (`int_stack`, `str_stack`, `programs`,
`pending`, `append_string`, `copy_string`, and so on).

5. One macro generating several commands

`H(x,y,z)` writes the "pop n and swap with the element n below" code and is
used for both `s` (integer stack) and `S` (string stack). `O(x,y)` writes the
code that reads a growing string up to a closing character and is used for
both `[...]` and `"..."`. Because the two copies only exist after expansion,
it is hard to see that two commands share the same logic. In simple.c the
string reader is a function `read_string()`.

6. Hidden input source

`W` expands to `while((c=k?K[--k]:getchar())-EOF)`, so every read either
takes a character from the pending buffer `K` or from standard input. Running
a program (`=x`) works by pushing its text into `K` backwards, so the main
loop "re-reads" it as if it were typed. The same `W` loop is also used just to
read one character and then `break`, which hides that `=` and `:` only read a
single label character. simple.c uses a `next_char()` function.

7. Short-circuit operators as if statements

Many commands are written as expressions like `p&&close(s[p--])` or
`wait(&s[++p])+1||(s[p]=-1)`. The `&&` only runs the right side when the left
is true and `||` only runs the right side when the left is false, so they act
as hidden `if` statements. Comma operators like `(c=s[p--],s[p]+=c)` pack a
pop and an operation into one expression. simple.c uses ordinary `if`
statements.

8. Reused variables

`c` is the current input character but is also used as a temporary for a
popped value (`P` is `p>1&&(c=u,`) and as the saved string stack height in
`!`. `n` is the "currently reading a number" flag but is also used to hold a
program length in `=`. The slot just above the stack top is used as the swap
temporary. These double uses make it hard to tell what a variable means at
any given line.

9. Overflow check hidden in a macro

`V` is `if(k>o||p>o||q>o)return 2;` with `o` only given on the compiler
command line (`-Do=250`). Likewise `M` and `X` are `malloc` and `free` set
with `-D` in the Makefile, so reading the source alone does not tell you that
memory is being allocated and freed.
