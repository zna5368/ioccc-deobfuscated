# 2006/birken

Difficulty: hard

1. One generic "pair" structure for everything

`typedef V*d; V{ d o,_; X*b; int c; };` declares a single node type with two
pointers, a string and an integer. Every data structure in the program is built
from it: the token list, the list of all wires, the list of relays, each wire's
list of attached relays, the symbol table used while expanding a `def`, the
argument lists, and the wires and relays themselves. You only find out what a
node means by following chains such as `A->o->_->_->_->_->o->c` (the value of a
relay's coil wire). simple.c gives each role its own struct (`token`, `wire`,
`relay`, `binding`, `wire_list`) with named fields.

2. Lists with an empty node at the front

Lists such as `l` (wires) and `m` (relays) grow by filling the `o` of the
current head and then putting a fresh empty node in front of it
(`l->o=a; l W l->_=_;`). That is why the code keeps skipping the first node
(`l->_`, `m->_`, `_=_->_`). The relay's four wires are pushed the same way, so
they come out backwards: the "off" contact is first and the coil is last.
simple.c uses normal lists and stores the four relay wires in named fields.

3. Macros that hide keywords

`Z`, `Q`, `J`, `N`, `X` and `V` stand for `if`, `else`, `while`, `return`,
`char` and `struct e`. `P` is `a->b` and `W` is `=f();`, so `S W` means
`S = f();` (allocate a node). `g(_,a)` compares two strings and treats NULL as
"not equal".

4. Shifted strings

`v(s, n)` returns a copy of `s` with `n` subtracted from every character.
`"Cvjme!dpnqmfuf;!&e..."` minus 1 decodes to
`"Build complete: %d components, %d nodes.\n"`, and the `\x0b` turns into the
newline. Every input token also goes through `v(B,1)`, so the keywords are
compared in shifted form: `"cde"` is `def`, `"qtm"` is `run`, `"q"` is `r`,
`"/"` is `0`, `"0"` is `1`, `"b"` is `c` and `"j"` is `k`. A `:` becomes `9`, so
`57-*P` (57 is `'9'`) tests for the colon that ends a parameter list. Numbers
are shifted back with `atoi(v(_->b,-1))`. simple.c keeps the tokens as typed
and uses the real words.

5. Hidden program inside a string

The output format string is `"&e;!&t\x0b\0nbjo)*|jou!j>1..."`. `v()` stops at
the `\0`, so only `"%d: %s\n"` is ever decoded. The text after it, shifted by
one, is a second C program,
`main(){int i=0;for(;i<40;i++)printf("%d ",i*i+i+41);}`, which prints values
of Euler's prime polynomial. This is the "hidden program" mentioned in the
README.

6. Character codes and nested ternaries in place of names

In the `run` line, `R=i-47?48-i?i-98?106-i?0:r:q:p:j()` checks the first
character of a token against 47, 48, 98 and 106 (shifted `0`, `1`, `c` and
`k`) and picks a fresh wire, the high wire, the clock or the inverted clock.
The upper-to-lower case conversion uses `65` and `90` for `'A'` and `'Z'`, and
`strchr(c,35)` looks for `'#'` to cut off comments. simple.c uses character
literals and a `switch`.

7. Meaningless and misleading identifiers

Globals `k` to `r` hold the output list, the wire list, the relay list, the
token list, the primitive relay definition and the `1`, clock and inverted
clock wires. Locals are named `_`, `S,I,M,U,L,A,T,O,R`, and `PI` is just the
buffer size 314. Function names like `j`, `u`, `s`, `t` and `h` tell you
nothing; in simple.c they are `new_wire`, `find_token`, `find_definition`,
`instantiate` and `propagate`.

8. Odd ways to write constants and simple control flow

`1>>1>1` is 0, `'\''/'\''` is 1 (a character divided by itself), and `!!!R`
is just `!R`. `J(!--G){ ... break; }` is a `while` that always breaks after
one pass, so it is really an `if`. `J(PI)` is an endless loop.

9. Source shaped as a wave, with split tokens

The code is laid out in a wave shape. To fit it, identifiers and keywords are
split across lines with backslash-newline (`m\` `alloc`, `ma\` `in`,
`p\` `rintf`, `s\` `trchr`), and a stray `/**/` comment is used as padding.
