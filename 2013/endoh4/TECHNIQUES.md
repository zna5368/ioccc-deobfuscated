# 2013/endoh4

Difficulty: Insane

1. Source code shaped like a picture

The whole file is laid out as the flat net of a tetrahedron, the paper shape
you would cut out and fold. Line breaks and spaces go wherever the shape needs
them and not where the code needs them, so statements are chopped apart at
random places and nothing is indented to show structure.

2. Data hidden in comments

Comments such as `/* 2E5-2E5,2E5` and `/* -2E5-2E5XX*/` look like filler, but
the program can read its own source as input, and it picks out every number
it can find. A C compiler ignores comments, but `scanf` does not. `2E5` means
200000, so these comments are the corners of a large tetrahedron. Even the
`199309` in the `_POSIX_C_SOURCE` line is used as a coordinate, and the `XXX`
letters only pad the comments to the right length.

3. Words split across lines

`i\` at the end of a line followed by `nt*` makes `int*`, and `s\` followed
by `izeof` makes `sizeof`. A backslash just before a line break joins the two
lines before the compiler reads them, so a keyword can be cut in half. A search
for `sizeof` will not find these.

4. Comments used as spaces

`#ifndef/**/S` and `#define/* 2E5*/O(p,q,i)` use a comment where a space should
go. A comment counts as a space to the compiler, which lets the shape be filled
in and makes the preprocessor lines hard to spot. Some lines even start with a
comment: `/* 2E5-2E5X*/#include<stdio.h>`.

5. Preprocessor lines in the middle of a declaration

`struct timespec R={0,1E6};int j,k,m,` ends with a comma, and the declaration
only carries on with `n,u,v,` more than ten lines later. All the `#define`
lines in between are removed before the code is compiled, so the two halves
join up. The variable list looks unfinished and the second half looks like it
belongs to nothing.

6. Macros that are not complete expressions

`D(e,f)` starts with `(c-a)?` and it is used as `i=e*e>(c-a)*D(b,c)` and as
`0>D(c,b)`. The text before the macro name is really the first half of the
condition inside it, and the macro also supplies the `;` and the statements
after it. The code has no sensible meaning until the macro is expanded by
hand. `C` is similar. In `p=C` only the first part of the macro is assigned to
`p`, and the rest of it is a comma list that runs on into the code after it.

7. Macro with hidden side effects

`p=C,q=C,r=C` looks like three variables getting the same value. Each `C`
also replaces the vectors `(a,b,c)` and `(d,e,f)` with new ones made by a cross
product, so all three get different results. Those results are the distance of
a point along three different directions.

8. Macros built out of macros

`O` subtracts a coordinate of two points, `B` uses `O` to make a 2D cross
product, and `E` uses `B` to make a 3x3 determinant. A short call like
`E(p,q,r,h)` turns into several hundred characters of code. The formula never
appears anywhere in full.

9. Short names reused for many jobs

Every name is a single letter, and most have several meanings. `p` and `q` are
the cosine and sine results, then depth and sideways distance, then `p` is an
area and `q` is a flag. `u` is a vertex, then an array index, then a position
along a line. `G` has its own parameters also called `p`, `q` and `s`, which
hide the globals of the same name, and it overwrites them while merging faces.

10. One array with several jobs

`P` first points to where `scanf` should store the next number, then holds the
points with noise added, and finally holds the 2D screen positions of one face.
`F` and `V` are one block of memory split in half by `V=F+2*N`, which hides the
fact that they are two separate lists.

11. One function that returns three results

`o(x)` has no return value. It leaves cosine in `p`, sine in `q` and a square
root in `s`, all global variables. A call like `o(c)` wants only the square
root and `o(K)` wants only the cosine and sine, and nothing at the call says
which.

12. Math library replaced by a loop in a `for` header

The source does not use `math.h`. `s=(s+x/s)/2` is Newton's method for a square
root, placed in the third part of a `for`, while `r*=3.14*x/++i` builds up the
terms of the Taylor series. Even terms go to cosine and odd terms go to sine
by way of `i%2`. Two unrelated calculations share one loop. The `3.14` is a
rounded pi, which turns an angle measured in half turns into radians.

13. Ternary operator and comma operator instead of `if`

`i?c+=*P**P,1<N++%3?o(c),b=b<s?s:b,c=0:0:scanf("%*s");` is a whole group of
nested `if` statements written as a single expression. `?:` picks a branch,
a comma puts several steps into one branch, and a `:0` means "otherwise do
nothing".

14. Program logic inside `for` headers

`for(;u=F[j][m]*3,o(K=...),...,m=F[j][m],m-n;)` has an empty body and does all
of its work in the condition. It is a long list of steps joined by commas, and
only the final `m-n` decides whether to loop again. In the line drawing loop,
the plotting is in the third part of the `for`, and the calculation of the
position is in the body.

15. Subtraction used as "not equal"

`c-a`, `m-n`, `a[q]-r`, `h-p&&h-q` and `i%W-3` are all tests. A result of zero
counts as false, so `x-y` is true when `x` and `y` are different. In the same
way `q+1` and `s+1` mean "is not -1". The reader sees arithmetic where there is
really a comparison.

16. Escape sequence built from scattered numbers

`T[3]=0x48`, `*T=033`, `T[01]=91` and `T[2]=060` spell the terminal command
`ESC [ 0 H`, which moves the cursor to the top left. The four characters are
given in hexadecimal, octal and decimal, in a mixed-up order, and are split
between two macros. They get stored again for every corner and every line, even
though storing them once would be enough.

17. Chained assignment through array indexes

`v[v[v[p]=q]=r]=p` uses the value of an assignment as an index. From the inside
out it does `v[p]=q`, then `v[q]=r`, then `v[r]=p`, which links the three
corners of a triangle into a loop.

18. Recursion through nested arguments

`return G(r,q,p,f,G(p,r,q,f,G(p,q,s,g,f+1)));` makes three recursive
calls. They run from the innermost one outwards, as each call's result is the
next one's last argument. The first of them does not look for a new triangle
at all, because it always finds the one that was just made, and that is how the
merge check runs.

19. Lookup string as a state machine

`"',"";;;,;'"[T[v]%8^m%2]` works out what a character cell becomes after one
more dot is drawn in it. It uses the character code of the current contents as
an index, and `^m%2` flips the lowest bit for the lower half of the cell. The
string is arranged so that the codes of space, `'`, `,` and `;` each land on
the right replacement. It is also written as two strings side by side, which
the compiler joins, so that it fits the shape.

20. The value of an assignment used as a flag

`i=e*e>(c-a)*(c-a)?s=a,a=b,b=s,s=c,c=d,d=s:0` looks like a swap, but `i` is
set to the value of the last assignment. It is later used as true or false to
say whether the swap happened, so a `double` that happens to be non-zero is
standing in for a flag. If that value is less than 1 away from zero, then the
flag ends up false even though the swap took place.

21. Loop that looks like a retry

`for(;u==v;){...}` looks as if it repeats until two different vertices are
found.  `u` and `v` both start at 0 so it is entered once, and `v=!u` makes
sure they are different after that, so it can never run twice. It is a block of
code disguised as a loop.

22. Code that does nothing

`m=u+1` is stored and then replaced before it is read. `u=a+.5` is done twice
because it is inside `D`, which is used twice. The screen position in `C` is
worked out three times, and the first two results are thrown away. `K+=I+0`
adds a zero. All of it gives the reader more to work through.

23. Screen size hidden in a comma

`H=(W=S)*2` looks like one calculation, but `S` is `70,23`, so it becomes
`H=(W=70,23)*2`. The comma stores 70 in `W` and then gives 23 as the value,
which is doubled. One macro passes two numbers without any sign of it.

24. Information carried between frames

`X=x;Y=y;Z=z;` saves the camera position at the end of a frame. The difference
from the current position is how the view is oriented, so there is no up
direction stored anywhere. It only exists as the change between two frames. On
the first frame the saved position is still zero, so every point lands in the
center of the screen and nothing is drawn.

25. Unbracketed macro

`#define U rand()*2./RAND_MAX-1` has no brackets around it, so using it in a
larger formula would give a wrong answer. The code writes `(U)/1E5` in the one
place that matters, and the brackets look unnecessary to anyone who has not
looked up the macro.

26. Implicit conversions to `int`

`u=a+.5` assigns a `double` to an `int`, which rounds a positive number to the
nearest whole number by adding a half and cutting off the rest. `q=time(0)<t+3`
stores a true or false result in a `double`. There is no cast or function call
to show either of them.

27. Global variables that rely on starting at zero

`Q` is passed to `realloc` with no setup, `c` and `b` are used for totals,
`j` is used as a loop counter, and `u` and `v` are compared before they get a
value. All of this works only because global variables begin as 0, and nothing
in the code says so.
