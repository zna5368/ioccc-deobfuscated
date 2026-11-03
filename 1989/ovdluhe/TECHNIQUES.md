# 1989/ovdluhe

Difficulty: Medium

1. Meaningless, look-alike identifiers

Almost every name is built from the letters `a`, `p` and `e`: the type `ape`,
the globals `a`, `ap`, `ae`, `p`, `e`, the locals `pe`, `ep`, `pa`, `Ap`, and
the function `PA`. Because they look so alike it is hard to tell the template
buffer (`a`) from the correlator string (`p`), the counter table (`e`), or the
end-of-template pointer (`ae`). In simple.c they are `template`, `context`,
`counts`, `template_end` and `pick_next_char`.

2. Keywords hidden behind macros

`#define EA register` and `#define EP unsigned` hide ordinary C keywords, and
`typedef char ape` hides the `char` type. A line like
`EA EP ape,ea=AE;` reads like nonsense until you replace the macros and see it
is `register unsigned ape, ea = 128;`.

3. Numbers built from bit shifts of 1

No number appears directly. `A` is 1, `AP` is `1<<1` = 2, `P` is `1<<2` = 4,
`AE` is `(4<<4)<<1` = 128, and `PE` is `((1<<4)<<4)<<4` = 4096. Even zero is
written as `E`, which is `(unsigned)1>>1`, and `-1` (the `EOF` check) is `-A`.
The value `~E` (all bits set) is stored into a `char`, which turns it into -1.
You have to work out each shift to find the real constants.

4. Typedef name reused as a variable name

Inside the `APE` macro, `register unsigned ape, ea` looks like it declares an
`unsigned ape` (an unsigned char). C does not let you add `unsigned` to a
typedef name, so the compiler treats `ape` as the name of a new `unsigned int`
variable that hides the typedef inside that block. The variable is never used;
the loop counter that matters is `ea`.

5. Code hidden inside a macro

`APE` is a whole block of code (a loop that zeroes the 128 counters) packed
into a macro, so `for(...){APE;if(...)` hides the clearing of the statistics
table. simple.c writes the loop out where it runs.

6. Misleading semicolon placement

The `typedef char ape` line has no semicolon. The missing semicolon is at the
start of the global variable line (`;ape a[PE+A],...`), after all the
`#define` lines. This makes the typedef look unfinished and the next line
look like it begins with a stray `;`.

7. Crammed expressions and side effects

The input loop `while(((*(ep++)=getchar())!=-A)&&((ep-a)<PE));` reads a
character, stores it, moves the pointer, checks for end of file and checks the
buffer size all in one condition with an empty body. In the same way,
`*(ae= --ep)=E` moves back one character, saves the end pointer and writes the
terminating zero all at once. In `PA()` the line
`pe=~E,Ap=!Ap;while((Ap+=e[++pe])<pa);` uses the comma operator, `!Ap` to
reset a non-zero counter to 0, and a loop with all its work in the condition.
simple.c splits these into separate, plain statements.

8. Undefined evaluation order

The shift of the correlator string, `*(ep-A)= *(ep++);`, reads and changes
`ep` in the same expression, so the C standard does not say which happens
first. simple.c uses a plain loop that copies `pos[0]` to `pos[-1]` and then
increments `pos`.

9. Implicit int and implicit declarations

`main()` and `PA()` have no return type and `srand`, `time`, `rand`,
`getchar` and `putchar` are used without their headers, relying on old C
rules that make anything undeclared return `int`. `PA()` is also called
before it is defined. This hides what each function returns.

10. Small integer types that silently wrap

The counters `Ap`, `pa` and the table `e[]` are all `char`, so with a large
template they wrap around past 127 into negative numbers. That quietly
changes which character is picked and can make `PA()` return 0 and stop the
program early. simple.c keeps the `char` types to produce the same output,
with a comment explaining why.
