# 2018/ciura

Difficulty: Insane

1. One big string holds all the data

`static unsigned char c[]="]\33[0m\33[7m[&&'% $ # ! ..."` is 2787 bytes long
and holds everything the program knows: the highlight marks, a table of
character classes, two state machine tables and two packed word lists. Parts
of the string are found by adding offsets such as `c+5`, `c+52`, `c+67` or
`1950[w+c]`. Because every byte is just a character in a string, nothing
tells the reader which part is text and which part is a table. simple.c
writes the highlight marks as normal strings (`"\33[7m["`, `"]\33[0m"`,
`"["`, `"]"`, `"\n"`) and turns every table into its own named array.

2. Numbers packed into characters

The state tables store two numbers in one byte. For a character state the byte
is `5*next_state + output`, so the program reads it back with `*w[k]%5` (the
output) and `*w[k]/5*8+3+c` (a pointer to the next state's row of 8 bytes).
The word lists store a check value as `2 + label + 28*output`, and the next
state is spread over three bytes that are decoded with
`c[w+1950] + 94*(c[2w+464]/3%5 + c[2w+465]%26/7)`. The high digits are written
as the characters `' '`, `';'`, `'{'` and `'}'`, so the string looks like C
code full of semicolons and braces. simple.c replaces these with tables of
named states and events, for example `{IN_WORD, SENTENCE_START}`.

3. A dictionary hidden as an interlaced sparse array

The roughly 450 word endings (such as `"ly"`, `" was"`, `"taken"`) are stored as
a compressed trie. Each state is an array of 28 slots (one per letter plus
"word start" and "apostrophe or period"), and the arrays of all states are
laid on top of each other so that they share the same memory. A slot only
belongs to a state if the label stored inside it matches the label used to
find it: `O-(c[w]-2)%28||...`. The word is read backwards, one letter at a
time, until a slot does not match. simple.c lists the word endings as plain
string arrays (`adverbs`, `participles`, `wh_words`, ...) and picks the longest
ending that matches, which gives the same answer.

4. One recursive function does everything

`int V(int w)` is the only function besides `main`. The value of `w` picks
the job: `V(0)` steps back over trailing spaces, `V(1)` skips leading spaces,
`V(2)` and `V(3)` run the two state machines, `V(4)` reads the next
character, `V(5)` looks up the class of a word, `V(6)` handles one event, and
any larger value is a state in the word trie (`V(808)` looks up a word class,
`V(205)` checks for an abbreviation). Loops are written as the function
calling itself, such as `V(w)` at the end of the space skipping code and
`V(154+...)` for the next trie state. simple.c gives each job its own
function and uses `while` and `for` loops.

5. Deeply nested conditional operators

The body of `V` is one `return` statement made of the `?:` operator nested
about twenty levels deep, joined by commas. `1<w?3<w?4<w?5<w?6<w? ... : ...`
is a chain of comparisons that acts like a `switch` on `w`. The parentheses
that close each part are far from where they open: for example the
`(v=1,l=*k,V(205)` group in `V(6)` keeps going for many more lines before its
`)` appears, which makes it easy to misread which statements belong to which
branch. simple.c uses `if` and `switch` statements.

6. Code formatted into a picture

The source is cut into short pieces laid out to form shapes, with line breaks
in the middle of numbers, names and operators such as `9<<` / `20`, `1[k` /
`]`, and `V(` / `205`. The layout says nothing about how the code is grouped.

7. A macro that hides function calls

`#define W ),write(1,` closes one expression and starts a call to `write`.
`(P=1 W L,o-L W c+s,S W o,*k-o W c,S)` expands to
`(P=1),write(1,L,o-L),write(1,c+s,S),write(1,o,*k-o),write(1,c,S)`, which
prints four pieces of text (the sentence before the passive part, the opening
mark, the passive part and the closing mark) while also setting `P` to 1.
simple.c writes the four `write` calls out on their own lines.

8. Reversed array indexing

`1[k]`, `2[k]`, `3[k]`, `w[k]`, `1950[w+c]` and `464[w+c+w]` swap the array
and the index. In C, `a[b]` means `*(a + b)`, and addition does not care
about order, so `1[k]` is the same as `k[1]`.

9. Single letter names and an array of pointers used as named variables

The variables `c`, `C`, `k`, `K`, `l`, `L`, `o`, `O`, `p`, `P`, `s`, `S`
and `v` look alike in pairs. The array `k[4]` holds four unrelated pointers:
`k[0]` is the reading position, `k[1]` the end of the previous word, `k[2]`
the sentence machine state and `k[3]` the passive machine state. `V(2)` and
`V(3)` share code by using `w[k]` to pick which of the two machine states to
update. Even `v` is reused: it holds a character, a character machine event,
a word class and a passive machine action at different times. simple.c names
them `cursor`, `token_start`, `sentence_state`, `passive_state`,
`sentence_start`, `highlight_start` and so on.

10. Variables that change meaning

`s` and `S` start as 0 and 1, so `read(s,2+C,9<<20)` reads standard input
and `isatty(S)` tests standard output. They are then reused as the offset and
length of the highlight marks inside `c`: `s=isatty(S)?S=5:9` makes `c+s` the
opening mark (`"\33[7m["` on a terminal, `"["` otherwise) and `c` with length
`S` the closing mark (`"]\33[0m"` or `"]"`).

11. Bit tricks to classify characters

`1&129>>O-39` checks whether `O` is an apostrophe (39) or a period (46):
`129` is `10000001` in binary, so shifting it right by 0 or 7 places leaves a
1 in the lowest bit. For other characters the shift is negative or too large,
which is undefined behavior in C, so simple.c tests `ch == '\'' || ch == '.'`
directly. `27>(O%=32)&&0<O?1+O:0` turns both `A`-`Z` and `a`-`z` into the
numbers 2 to 27, because upper and lower case letters differ only by 32.
The character class of the next byte is found with
`39-c[v>>7?15:1-v/32?10-v?(312-v%32)/26+v/32*2:13:82-v]`, a formula that
squeezes 256 characters into a 41 byte table. simple.c uses a function with
plain comparisons such as `ch >= 'A' && ch <= 'Z'`.

12. Rereading characters by moving the pointer back

When a sentence ends, `V(0)` moves the reading pointer back over the spaces
before the character that ended it (`*K-=!w` subtracts 1 only when `w` is 0).
The next calls read those characters again, so the capital letter that ended
one sentence is also the one that starts the next. simple.c does the same with
a `do ... while` loop and explains it in a comment.

13. Work hidden in a for loop

`for(s=isatty(S)?S=5:9;v;V(2))V(6);` sets up the highlight marks in the
initialization part, reads the next character in the increment part, and
handles the event in the body. The loop stops when the character machine
reports end of text (`v` becomes 0). The exit status `P` is 1 only when a
passive sentence was found.
