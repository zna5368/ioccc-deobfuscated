# 2011/richards

Difficulty: Terrifying

1. JIT built from the compiler's own machine code

The function `t()` holds a small piece of C for every dc command (`+`, `-`,
`*`, `_`, `=`, `<`, `>`, `!`, `r`, `d`, `s`, `S`, `l`, `L`, `/`, `p`, `x`, `|`)
plus one piece for every number that can be pushed. The pieces are separated
by calls `S r("+")`, `S r("-")` and so on. When the program starts, `main()`
runs `t()` once with `d->r` set to `re()`. `re()` calls `rd()`, which searches
the stack for its return address back into `t()`. Everything between the
previous return address and this one is the machine code for that command, so
its start and length are saved in `x[]` and `y[]`. Compiling a line of input
(`K()`) is then just copying these pieces of machine code one after another
into a buffer from `mmap()` with the `W()` macro. There is no assembler
anywhere, but the program still builds real machine code at run time.

2. Code written to survive being copied

For a piece of machine code to work after being moved, it must not depend on
its own address. The struct pointer `d` is `volatile`, so every piece reloads
it from the same stack slot instead of keeping it in a register. Anything that
would need a normal function call (division, `printf()`, running a macro) is
done through the function table `S l[...]`, because a direct call uses an
address relative to the current instruction. The last line of `t()` calls
`S l[0]` only to stop the compiler from turning the final `S r("")` call into
a jump, which would leave no return address to find. The first piece (the
function's start, id `1`) and the last piece (its end, id `2`, copied up to
the start of `T()`) are put at the start and end of every compiled line.

3. Recording and running with different function tables

The same calls in `t()` do different things depending on what the struct
holds. While recording, `S r` is `re()` and `S l` is `lib1` (every entry is
the empty function `T`), so nothing is printed or divided. Before running,
`main()` switches `S r` to `T` and `S l` to `lib2` (`ld`, `lp`, `lx`, `l1`).
The empty function `T()` also does a second job: because it comes right after
`t()` in memory, its address marks where `t()`'s code ends.

4. Macro-generated table of 729 numbers

`G` expands through the macros `E`, `D`, `C`, `B` and `A` into 729 calls of
`O(x,y,z)`. The first definition of `O` pastes the arguments into six digit
numbers like `111111, 111112, ... 333333`, which fill `m[]`. `main()` builds
the reverse table with `n[m[i]] = i`. Later `O` is `#undef`ined and redefined
as `*++M=S m[(x*S h+y)*S h+z]; S r("");`, so the same `G` now makes 729
code pieces in `t()`. With `S h` set to 100, `(x*100+y)*100+z` rebuilds the six
digit number, and looking it up in `n` gives back `i`, so piece `i` pushes the
number `i`. The empty string tells `re()` to name each piece with the next
number from `m[]`, which can not clash with the one character names.

5. Numbers built from small pushes

`g()` writes code to push any number. Numbers up to 256 have their own piece.
Larger numbers are split into base 256 digits: `g(v / 256)`, push 256, `*`,
`g(v % 256)`, `+`. So `1000000` becomes several pushes, multiplies and adds.

6. Return address found by scanning the stack

`rd()` starts at its local variable `a` and walks up to 32 words of the stack,
looking for a value that lies between the current position in `t()` and the
address of `T()` (skipping the address of its own argument `v`). If none is
found (some CPUs keep the return address in a register), it uses `alloca()`
to take more stack and `setjmp()` to force registers onto the stack, then
calls itself to try again. `rd()` is called through the volatile function
pointer `rs` so the compiler can not inline it and change the stack layout.

7. Comparison operators flipped with a bit trick

dc's `<r` runs register `r` if the top value is less than the one below it,
but the code piece for `<` compares the other way round. `K()` fixes this with
`c ^= 2`, because `'<'` and `'>'` differ only in that bit. The test
`if (!(c & 1))` (true for `<` and `>`, false for `=`) is put around a `case`
label so `!<`, `<` and `>` all jump into the middle of the same statement.

8. Case labels hidden by macros

`I` is `case` and `N` is `break; case`, so `N '[':` ends one case and starts
the next. Combined with fall through (`'_'` falls into `default`, `'!'` falls
into `'<'`, `'>'`, `'='`) the `switch` in `K()` is hard to follow.

9. Short macros and typedefs for common words

`S` is `d->`, `M` is `d->a` (the stack pointer), `q` is `typedef`, `k` is
`char`, `o` is `void`, `u` is `int`, `f` is a function pointer type, `vo` is
`volatile` and `J(i)` is `calloc(i,1)`. `F(x)` starts a library function named
`l` + `x`, and `Q(x)` writes a whole binary operator piece such as
`M--; *M = *M + M[1]; S r("+");`. `R(x)` turns its argument into a string with
`#x`, so `R(==)` passes `"=="` whose first character `'='` names the piece.

10. Two thirds of a swap

`M[-1] ^= *M; *M ^= M[-1]` looks like the classic xor swap but leaves out the
last step, so `r` copies the second value to the top and leaves the xor of the
two values below it.

11. Misleading formatting

The library functions start on the same line as the closing `}` of `re()`
(`} F(d) M--;`) and their bodies are pushed against the left margin, so it is
hard to see where one function ends and the next begins.

12. Hidden constants

`H` is `9<<16` (589824), which is the size of almost every table, `l` is the
global `256` and `10` is used instead of `'\n'` in the comment skipping loop.
The `mmap()` protection is `-1`, which turns on every bit (read, write and
execute).
