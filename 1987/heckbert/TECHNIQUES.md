# 1987/heckbert

Difficulty: Medium

1. Macros that hide keywords and library calls

`#define _ define` makes `#_` mean `#define`, so `#_ A putchar`, `#_ B return`
and `#_ C index` turn `A(10)`, `B x;` and `C(j,10)` into `putchar('\n')`,
`return x;` and `strchr(j,'\n')`. The reader has to translate every one-letter
name back to the real function or keyword. (The build also passes
`-Dindex=strchr`, so `index` itself is another layer of renaming.)

2. Single letter global variables

Almost every variable (`r`, `c`, `d`, `i`, `l`, `j`, `m`, `k`, `n`, `h`, `y`,
`e`, `u`, `v`, `w`, `f`, `p`, `s`, `x`) is a one-letter global. Three tokens
are tracked at once as pairs of pointers (`i`/`l` for the previous token,
`j`/`m` for the current one and `k`/`n` for the next one) together with their
types in `u`, `v` and `w`. Nothing in the names says which is which. In
simple.c they are `prev_start`/`prev_end`, `cur_start`/`cur_end`,
`next_start`/`next_end`, `column`, `folding`, `width` and so on.

3. Implicit int and old style (K&R) declarations

`e,u=1,v,w,f=1,p,s,x;` declares integers without ever writing `int`, and
`main(a,b)char**b;{` and `g(j,m)char*j,**m;{` put the parameter types between
the parameter list and the body. Library functions such as `read`, `atoi` and
`strncmp` are used without any declaration. This makes the declarations hard
to spot.

4. Magic numbers instead of character literals

Characters are written as their ASCII codes: `10` is `'\n'`, `32` is a space,
`35` is `'#'`, `46` is `'.'`, `61` is `'='`, `34` and `39` are the quote
characters, `42` is `'*'`, `47` is `'/'`, `92` is `'\\'`, `95` is `'_'` and
`101`/`69` are `'e'`/`'E'`.

5. Unnamed token type codes

The tokenizer `g()` returns plain numbers 0 to 4 for word, punctuation, end of
input, white space and comment. The main loop then tests `v==1`, `v!=2`,
`v==3` and even `v>2` (which means "white space or comment"). simple.c gives
these codes names such as `TOKEN_WORD` and `TOKEN_SPACE`.

6. Packed table of operators

`d=">=<=!===||&&->++-->><<"` looks like line noise but it is eleven two
character operators written back to back. The loop `for(y=d;*y&&strncmp(y,j,2);y+=2);`
steps through it two characters at a time to see if the input starts with one
of them.

7. Comma operator and short circuit evaluation used as statements

Lines like `e&&A(10),e=f=0;` and `B*m=h,0;` pack several actions into one
expression. `e&&A(10)` only prints a newline when `e` is not zero, because
`&&` stops as soon as the left side is false, so it acts as a hidden `if`.
`return *m=h,0;` stores the end pointer and then returns 0.

8. Long condition chains and dangling else

The main loop is a chain of `if`/`else if` with very long conditions such as
`v>2&&(u||w)&&(f||u)&&(l-i>1||*i!=61||n-k>1||!C("-*&",*k))`, and the
`else if(v==3)if(...)...;else ...;else{...}` part relies on each `else`
pairing with the nearest `if`. In simple.c each condition is split up and
commented.

9. Tricky loop bodies and hidden side effects

`for(k=m;v!=2;j=k,m=n,v=w,k=m)` shifts the "next" token into "current" inside
the loop's increment part, and `continue` is used to skip the code at the end
of the loop that remembers the previous token. The comment scanner
`while(*++h!=42||*++h!=47);` moves the pointer in both halves of the condition
and has an empty body, so all the work happens in the test. Empty-body loops
like `for(h=j;h<r&&C(" \t\n",*h);h++);` are also easy to miss.

10. Pointer comparison with a possibly NULL result

`(char*)C(j,10)<m` asks whether a newline appears before the end of the
current token by comparing the pointer returned by `strchr` with `m`. If no
newline is found `strchr` returns NULL, which also compares as smaller, so the
test silently succeeds in that case too.

11. Dense formatting

The whole program is squeezed into a rectangle of lines with no indentation,
and statements are split across lines in odd places (for example `A` at the end
of one line and `(10)` at the start of the next), hiding the structure of the
code.
