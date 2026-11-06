# 2024/endoh2

Difficulty: Insane

1. Physics parameters supplied only on the compiler command line

`W`, `H`, `dt`, `e`, `u`, `rpm` and `tilt` are never defined in `prog.c`.
They come from `-D` options, so reading the source alone does not tell you
that `e` is the coefficient of restitution or that `u` is friction. In
`simple.c` they are given readable aliases such as `RESTITUTION` and
`FRICTION`.

2. One-letter names and recycled global variables

Almost everything is a single-letter global: `h`, `k`, `z`, `Z`, `i`, `a`, `b`
and `E` are each reused for many unrelated jobs. For example, `Z` is the tilt
angle, then the angle around a ring of particles, then the determinant of the
inertia tensor, then the deepest floor penetration. `i` is both a random seed
and a screen index. `v` holds the inverse-inertia response, then a point
velocity, then a contact force, then the change in angular velocity, all in
the same loop.

3. Quaternions used for all vector math

`typedef struct Q{S a[4];}Q` stores 3D vectors, scalars and rotations all as
quaternions. A single multiply function `m()` gives the cross product (vector
part) and minus the dot product (real part) at once. `s()` scales a vector by
building the quaternion `(0,0,0,b)` and multiplying, and `m(Y,Y).a[3]` is a
sneaky way to get `-|Y|^2`. The inverse inertia tensor is built from cross
products of the tensor rows (`m(t[(i+1)%3],t[(i+2)%3])`).

4. Multiplication table packed into magic numbers

The 16 terms of the quaternion product are produced by one loop. The number
`19995` holds, two bits at a time, which component of `a` to use, with the
`k>7?15-k:k` trick to reuse 8 entries for all 16. The number `5432` holds
the plus/minus signs: `1-(5432>>k&2)` turns a bit into `+1` or `-1`.

5. Hand-made math functions instead of math.h

`o()` computes sine and cosine with a 99-term Taylor series, flipping the sign
of the term each step and writing into `p[k%2]` so odd and even terms go to
different outputs. `K()` computes a square root with 99 steps of Newton's
method, leaving the answer in the global `h`. Their results are passed back
through globals instead of return values.

6. Magic constant approximations

`71./8136` is pi/360 (degrees to half-angle radians), `71*rpm/13560.` is
rpm times pi/600, `Z*113<710` means "angle < 2*pi" (710/113 is close to
2*pi), `36476` is (600/pi)^2 for converting back to rpm, and `47`, `64`,
`58`, `45` and `72` are the character codes of `/`, `@`, `:`, `-` and `H`.

7. Work hidden inside for-loop headers and comma expressions

The particle-creating loop does nothing in its body except `Z+=z=2./a`; all
the real work (radius, mass, sin/cos, center of mass, random number) is a long
comma list in the increment part. The screen-clearing loop puts the real
assignment `C[i]=...` in the increment part too. Assignments are buried in
conditions such as `j=C[a++],j&&j!=47`.

8. Bit tricks for character meaning

`j%2` decides if a character is a heavy or light particle (its lowest bit),
`j>32` skips spaces and control characters, and `(a&16)+45` makes the floor
alternate between `-` and `=` every 16 columns.

9. Misleading no-op tokens and comments

`/1`, `/+1`, `0/1`, `1/+h`, `4/+1` and empty comments `/**/` are sprinkled
through the code to break up the shape without doing anything. `us\` +
newline + `leep` splits the name `usleep` across two lines using a
line-continuation backslash.

10. Shaped source code

The code is laid out as a picture of a spinning top, which forces strange line
breaks in the middle of expressions and names.

11. Pointer type punning

`E=(S*)p` treats a particle struct as a plain float array so that `*E` and
`E+1` reach the x, y and z of its position. `E=p->v.a` and `E=v.a` do the
same for other vectors.

12. Shared buffers and sentinels

Input lines are read with `fgets` straight into the screen buffer `C`, which is
later overwritten by the picture. The escape sequence `"\x1b[H\x1b[J"` at the
start of the buffer clears the screen once, then `B[5]=72` changes the `J` to
`H` so later frames only move the cursor home. Particle loops stop at the first
particle whose mass is zero instead of using a count.

13. Repeated work that hides the structure

The inertia tensor and its inverse are recomputed 20 times every frame even
though the body never changes shape. `simple.c` computes them once.
