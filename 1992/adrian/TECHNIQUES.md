# 1992/adrian

Difficulty: Hard

1. Source code that is also the program's input

The program's default automaton file is its own source (`__FILE__`). The first
line `/* . wc . */` and odd comments such as `/* Wht majr flwchrt? */` are both
C comments and automaton rules. Running the program on its own source keeps only
the lines matching that automaton, and those lines form other programs (`wc`,
`echo`, `head`, `basename`, `sleep`). Every line therefore has two jobs, so
characters were placed to satisfy the automaton rather than to be readable.

2. Hidden programs mixed into the main program

Code for the other programs is spread around the file and switched off in the
main program. For example `main(int sl,char *j[]){ sleep(...)` sits inside a
comment in `A()`, the `basename` code is passed to a macro `v(...)` that is
redefined to expand to nothing, and the `echo` program is the unused function
`O()`. The `wc` variables (`wcl`, `wcw`, `wcc`) are updated but their result is
only printed under `if(j+28)`, which is always false.

3. Encoded error messages

The string `"}|uutsrq`_^bji`[Zk..."` in `E()` holds three error messages
interleaved. Starting at index 0, 1 or 2 and stepping by 3, each character has
`index - 9` added to it, giving `too_many_states`, `too_many_rules` and
`no_destination`.

4. Arithmetic that hides constants

`j` is `-28`, so `j +j+*"* . basename" +j+*"* r ] "` is
`-28*3 + '*' + '*'`, which is `0`. The expression just stores `N(m,s)` in two
variables. `while(j)` is really `while(1)` because `j` is never zero.

5. Meaningless and dead expressions

Lines such as `echo| m^ e| 5| (int) .8| echo| wc |C ==o[o[C] .e] . e| ...`,
`head; W = .1* head;`, `0&& W && wcl++ < 10 && printf((W,Y));` and `W, jr;`
compute values that are thrown away or never run. They exist only so the right
words appear on the right lines for the hidden programs.

6. Misleading names and macros

Variables are named after unix commands (`wc`, `echo`, `head`, `basename`,
`jr`, `sl`) even though they have nothing to do with them, `W` is both a global
`int` and a local `FILE*`, and `gets(Y)` is a macro that really calls the safer
`fgets`. Short macros (`P`, `G`, `z`, `S`) hide the table sizes and a
string-copy helper.

7. Array index trick

`i["..."]` is the same as `"..."[i]` because `a[b]` means `*(a+b)` in C, and
addition works in either order.

8. Unusual formatting

Expressions are broken across many lines with odd indentation, such as
`o[ c].w[i] .g[0 ] =='.'`, so that the code is hard to scan and the line
layout serves the hidden-program filters.
