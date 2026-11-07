# 2024/ferguson1

Difficulty: terrifying

1. Goto spaghetti

The whole game is one `main()` held together by about 150 `goto`s, and many of
them only jump to another `goto`. For example `if(x(0,1))goto I;` goes to
`I:goto a;`, then to `a:goto H;`, then to `H:goto R;`. To find out where the
code really goes you have to follow every link in the chain. The game loop
itself has no `for` or `while`. It is a label `yy:` at the top of a block and
`if(A++<21) goto yy;` at the bottom.

2. Labels hidden in unrelated code

Labels are put wherever there is space, often inside an `if` or `else` block
that has nothing to do with them. `F:V(51); R+=18; goto L;` (the "shot a bald
eagle" result of hunting) sits inside the `if(b<1||M<0||M>2169)` game-over
test, right after a `goto _;`, so the only way in is a jump. The death macro
`w(i)` ends in `goto hell;`, so any label written after it can only be
reached by a jump. `U:w(24) K:R-=...` uses this to put the "eat food" code
right after the "starve to death" code.

3. Jumping into the middle of loops

Simple loops are broken into pieces that jump around. The loop that adds what
was bought at a fort to the supplies is `A:for(X=1; X<6; ){ goto BB; ... }`.
`BB:` adds the item, jumps to `HA:` to subtract the price, which jumps to
`TA:++X; goto at;`, and `at:` is the end of the loop body. Each piece is stored
in a different loop or block. The loop that sets negative supplies back to 0
(`P:`) works the same way and uses `kk:`, `LL:`, `uu:` and `ZZ:`.

4. Labels inside switch statements and cases that are elsewhere

Plain labels are mixed in with the `case` labels of a switch, e.g. `M:V(1);`
and `v:memset(...)` inside `switch(r)`, and `q:R+=40; goto Y;` before `case 1:`
of a nested switch. The code for some cases is not under the case at all:
`case 6:goto g;` jumps to `g:goto S;` near the game-over code, which jumps to
`S:` in yet another switch where the wagon fire event really is.

5. Fall-through between cases and nested switches

`case 8:` (swamped fording a river) has no `goto` at its end, so it runs on
into the label `V:switch(x(1,9))`, which `case 11:` also jumps to. Inside that
switch, `case 4:i=17;` of the innermost switch runs on out of the switch and
into `case 2:V(5);` of the switch around it. The value of `q` is then checked
again to work out which event started it.

6. Dead code and impossible branches

Some code can never run. The `w(i)` macro has a `goto M;` after `goto hell;`.
`goto heaven;` comes straight after another `goto`. `M-=17;` comes after a
switch whose every case jumps away. A whole "hunt at the fort" block inside
`if(z)` is never reached by running into it, because `z` is always 0 there.
Its labels (`l:`, `qq:`, `zz:` ...) are still used by the normal hunting code.
Two random calls exist only to use up a random number:
`if(x(0,1))goto I;` ends up at `R:` either way, and `if(!x(0,9)){goto r; ...}`
ends up at `c:` either way.

7. Strings and format strings stored in a data file

The program has no text in it. Every message is a numbered block of 1663 bytes
in the `data` file. The whole file is read into `y`, and `V(n)` prints block
`n` (`&y[Z(n)]`, where `Z(n)` is `n*1663`). The `printf()` and `scanf()` format
strings are blocks too: `printf(&y[Z(11)],R,E,G,O,N,T,b,M)` prints the
status line, and the `S(x,z)` macro reads input with format `&y[Z(3)]`, which
is `"%ld"`. You cannot see what the program prints or reads without decoding
the data file.

8. Reading NUL bytes with getdelim()

`getdelim(&y,&J,EOF,K)` uses `EOF` as the end-of-line character. No byte in the
file is `EOF`, so this reads the whole file, NUL bytes included, into one
buffer. The NUL bytes end each message inside its block.

9. Compound literal arithmetic

`X<--(unsigned){ 7 }` makes an unnamed `unsigned` with the value 7 and
decrements it, so it is the same as `X<6`. `V(X+--(unsigned){ 6 })` is the
same as `V(X+5)`, which prints purchase prompt 6 to 10 for item 1 to 5.

10. Names that spell words and confusing names

The variables are declared as `O,R,E,G,O,N, T,R,A,I,L` to spell OREGON TRAIL
(declaring the same variable twice is allowed). The supply table
`*o[]={ &O,&R,&E,&G,&O,&N}` also spells OREGON, so `&O` (oxen) is in it twice.
All names are one letter, and the same letter is often used for different
things: `x` is a function and a label, `W` is a variable and a label, `O` and
`R` are variables and labels, and `M` is the miles and a label. `X[t]` is
used in place of `t[X]` (both mean the same thing in C).

11. Message numbers used as flags

Flags are set inside the call that prints a message, e.g. `V(a=76)` prints the
"bad water" message and also sets the illness flag `a` to 76, and
`V(f=79)` sets the injury flag. `V(B=38)` remembers that you could not pay the
doctor, and `i=17` stores the number of the message to print when you die.

12. Macros that hide control flow

`w(i)` looks like a function call but prints a death message and the
tombstone and then does `goto hell` (`return 666`). `S(x,z)` hides a
`scanf()` loop that throws away bad input.

13. A switch used only to jump to the start

`main()` begins with `switch(1) {` and the first code inside it is the end of
the purchase code (`j:` and `B:`). Since only a `default:` label exists, the
program jumps over that code to `default: srand(time(0));`. This makes the
real start of the program hard to find.

14. A label inside a while loop

`while(i)heaven:{ w(i) goto _; }` is a loop that runs at most once. When `i`
(the spaghetti poisoning) is set, the player dies. The `heaven:` label can
only be reached by the dead `goto heaven;`.

15. Misleading layout and harmless extra code

The code is laid out in odd shapes with tabs, big gaps and lines split in the
middle of expressions like `R>23?23:R>18?18:R>13?13:R`. `T=1666-O-R-E-G-0-N`
has `-0` in it only so that it reads OREGON.
