# 2001/williams

Difficulty: Hard

1. Shape of the source code

The code is laid out as the nuclear warning symbol (three blades around a
circle). The layout breaks statements across lines in odd places, such as
`M` and `(b,l.s>>9,h,6)` being on separate lines, so the code is hard to
read.

2. Macros to shorten common code

`#define F return`, `#define R while(` and `#define Y if(l.t` replace common
keywords, so `Y==2)` is really `if(l.t==2)` and `R i--)` is `while(i--)`. The
macro `Y` hides half of an `if` condition, so the brackets do not look
balanced. `N(r)` is `random()%(r)`. Bigger macros hide whole pieces of logic:

- `U` stores a new object in the ring buffer (`add_object()` in `simple.c`).
- `K` restarts an object from its first position with type 0 so that the
  rest of the loop redraws (erases) its trail.
- `E` tells the parent of a split missile that one child is gone
  (`child_finished()`).
- `M(a,e,i,o)` sets the first point of a shape and draws it as a polygon.

3. Single letter names

Every variable and struct field is one letter. The struct
`{ O b,f,u,s,c,a,t,e,d; }` spells "b f u s c a t e d" ("obfuscated" without
the `o`) and the type `O` is just `long`. The global `I` (the starting copy of
every object) is shadowed by the parameter `C I` in `di()`, and the global
`p` is shadowed by locals in `nL()` and `di()`, so the same letter means
different things in different functions.

4. One field, several meanings

The struct fields are reused for different kinds of objects. For lines `a`
and `b` are the x and y step, but for explosions `a` is the current radius and
`b` is the largest radius. Field `e` is the index (plus one) of the parent
missile, but for smart bombs it is the x position they aim at. Field `t` is a
type number: 0 dead, 1 missile, 2 smart bomb, 3 erase trail, 4 ABM, 16 growing
explosion and 8 shrinking explosion. `I[i].t-=8` moves an explosion from
growing to shrinking to dead. `simple.c` uses unions and named constants.

5. XOR drawing to erase

The graphics context uses function 6 (`GXxor`), so drawing the same pixels a
second time removes them. The program never erases with a background color.
Missile trails are erased by drawing them again from the start (the `K`
macro), explosions are drawn twice with the same sizes, and destroyed cities
are removed by drawing them again with `dC(c)`.

6. Magic numbers for X11 constants

X11 names are written as plain numbers: `4` is `ButtonPressMask`, `1<<15` is
`ExposureMask`, `829` is the bit mask
`GCFunction|GCForeground|GCBackground|GCLineWidth|GCLineStyle|GCFillStyle|GCFillRule`,
`XFillPolygon(...,1,1)` is `Nonconvex, CoordModePrevious` and `90<<8` is the
full circle `360*64` in the 1/64 degree units that `XDrawArc` uses. The short
arrays `T` and `b` are really arrays of `XPoint` (x, y pairs) that are cast
with `(void*)`.

7. Fixed point arithmetic

All positions and speeds are multiplied by 512 (shifted left by 9 bits) so
that they can have a fraction without using floating point. That is why
`<<9` and `>>9` appear everywhere: `<<9` turns a pixel into the fixed point
form and `>>9` turns it back into a pixel.

8. The `(x|1)` tricks

`x|1` sets the lowest bit, so the number is always odd. `A/(x|1)` uses this to
never divide by zero. `(x|1)%2` is `1` for positive numbers and `-1` for
negative numbers (in C the remainder has the sign of the left side), so
`(x|1)%2*x` is the absolute value of `x`.

9. Short circuit operators as `if` statements

Many `if` statements are written as chains of `&&`, for example
`v%2&&hi(...)<0&&A++` and
`XCheckMaskEvent(d,4,&e)&&A&&--S&&nL(...)`. The right side only runs if
everything to its left is true. `(a[1]=i)&&XFillPolygon(...)` uses an
assignment as a test, so a smart bomb at y = 0 is not drawn. `Z|dL()` uses a
bitwise or, so `dL()` is always called even when `Z` is not zero.

10. Undefined order of evaluation

`I[n++]=L[n]=l` and `XDrawArc(d,w,g,(l.s>>9)-++l.a,h-l.a,l.a*2,l.a*2,...)`
change a variable and read it in the same expression, so the result depends
on the compiler. With GCC the first stores into slot `n` of both arrays and
then adds one, and the second only uses the bigger radius in the x position.
`simple.c` writes out the order GCC uses.

11. Implicit declarations and return types

None of the functions declare a return type, so they return `int`, and
`random()`, `srandom()`, `time()`, `sprintf()` and `puts()` are used without
including their headers. `dC()` and `Ne()` never return a value.
