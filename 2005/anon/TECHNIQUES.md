# 2005/anon

Difficulty: terrifying

1. Keywords redefined as macros

`return`, `do`, `while`, `for`, `if`, `switch` and `case` are all `#define`d
to mean something else. `return` becomes `return do`, `do(x,y)` becomes
`(a-x||y)&&`, and `switch(x)` is really the code that slides a tile. The
source looks like normal structured C with loops and branches, but after the
preprocessor runs there is not a single real loop, `if` or `switch` left.

2. The whole program is one expression

After expansion, `main()` is a single `return` of one huge boolean
expression. There is only one semicolon in the file. Branching is done with
short-circuit evaluation: `a && b` only runs `b` when `a` is true, and `a || b`
only runs `b` when `a` is false. So `if (a) b;` is written `a && b ||`. The
`if(p,q,r,s)` macro builds a full if/then/else this way.

3. Statements that are always false

To chain statements with `||`, every statement must evaluate to false. The
code forces this: `W(x,y)` stores a value and then does `& 0`, `pr` is
`!printf` (printf returns a positive count, so `!printf` is 0), and
`sscanf(...) & 0` throws the result away. The tile move ends with `| 1` for
the opposite reason, so that the following `&& f(13)` redraws the board.

4. main() as a state machine

`main()` is the only function. The `argc` parameter (`a`) is used as a state
number, and every `do(x, code)` section only runs when `a == x`. Calling
`main(13, b)` means "call the render function". Each section is written as
`(a - x || code)`: when `a` is not `x`, `a - x` is non-zero and the code is
skipped. The command line argument count picks the first state, which is why
1 or 2 arguments print the usage and more than 3 arguments jump into random
states and crash.

5. Loops by recursion

The `while(x,y,z)` macro defines state `x` as "if `y`, run `z` and then call
`main(x, b)` again". The `for` macro sets the counter, calls the loop state,
then closes the current state and opens the loop state behind your back, so
the loop body ends up after the code that follows it. The main game loop and
the "pick a new random number" retry are also recursive calls. Every key
press makes the stack deeper, which is why the program eventually crashes.

6. Memory Allocated by Stack Smashing

The program has no variables or arrays besides `argc` and `argv`. States 6
and 7 call `main()` recursively until the distance between the first and
the current `argc` is at least 4096 `long`s (`D >> 12`). That stretch of the
stack, full of dead stack frames that will never be returned to, is then used
as a `long` array (the "MASS"). From then on `argv` points to this array and
`R(i)` and `W(i,j)` read and write slot `i` of it. Slots 12 to 15 hold the
board size and hole position and the board itself starts at slot 32.

7. Detecting the stack direction

State 5 compares the address of its own `argc` with the caller's `argc`
(`C < p(b)`) to find out whether the stack grows down or up. The answer
(0 or 1) is added to 6 to pick state 6 or 7. The `T(x,y)` macro then uses the
lowest bit of the state number (`a & 1`) as a multiplier to choose between
two values without using `?:`.

8. Recovering argc and argv after they are lost

Once deep in the recursion, the original `argc` and `argv` are not
accessible. The code knows the address of the first `argc` and computes the
address of the first `argv` by adding the distance between `&b` and `&a` in
the current frame (`Q(&b) - Q(C)`), because the two parameters are the same
distance apart in every frame. Both are copied into slots 8 and 9. `A(i)`
then reads `argv[i]` through slot 8.

9. Operator precedence tricks

`a - 1 >> 1 ^ 1` is false only when `argc` is 3 or 4, and `a - 1 & ~1` is
false only when `argc` is 1 or 2. `(a & ~1) - 6` matches both 6 and 7. The
parity of `argc` (`R(9) & 1`) decides between a 2D and a 3D board. A 2D board
is turned into a 3D board one row deep by moving the second number into the
height and setting the depth to 1.

10. Misleading fragments

Lines such as `1 >> 1 ^ 1`, `1 & ~1` and the lone numbers `(14)`, `(9)`,
`(17)` and `(19)` look like meaningless statements but are pieces of the
state checks and leftovers of the `for` macro. Indentation does not follow
the real structure at all.

11. Counting digits by writing into argv[0]

The width of a tile is found with `snprintf(argv[0], 1, "%ld", size)`.
`snprintf` returns how many characters it would have written, so this gives
the number of digits, while only overwriting the first byte of the program
name with a terminating zero.

12. Exiting through the stty result

State 15 is `system("stty echo") || (exit(0), a)`, so the program only exits
when `stty` succeeds. If it fails, or if `stty cbreak -echo` fails when the
game starts, the expression returns to the dead stack frames that were
overwritten by the MASS and the program crashes.
