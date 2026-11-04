# 2005/klausler

Difficulty: Medium

1. Dictionary pulled in from outside the source

`char*x[]={ #include "dict.h" 0};` builds the word list from a header that
`make` creates from the system dictionary (`/usr/share/dict/words`). The
Makefile lower-cases each word, removes anything that is not a letter, drops
one-letter words except `a`, `i` and `s`, and wraps every word in quotes with a
comma. Thousands of strings become part of the program without being counted
in the source size, and the source alone does not show what data it uses.

2. One-letter names

Every variable, type and function has a one-letter name: `c` is the count of
each unused letter, `h` and `l` are the first and last word of each length, `s`
holds the chosen words, `Z` is a pool of list nodes, `F` is the next free node,
`m` is the number of words wanted, `a` is the exit status, `k` takes letters
away, `u` gives them back and `S` is the search. `B` is `unsigned char` and `L`
is the list node type.

3. A check that always changes the counts

`k()` lowers the count of each letter of a word, even when a letter is missing.
`g|=!c[*p++]--` sets `g` when a count was already zero before it was lowered.
The function does not undo the change itself, so every caller must call `u()`
afterwards. This is hidden in the third part of the `for` loops:
`for(;p=x[i++];u(p))` and `...;u(w->s),w=w->n)`.

4. Bit trick to choose a return value

`return g-1&p-q;` means `(g-1) & (p-q)`. When no letter was missing, `g` is 0
and `g-1` is -1, which has every bit set, so the result is `p-q`, the length of
the word. When a letter was missing, `g` is 1 and `g-1` is 0, so the result is
0. This replaces an `if` statement.

5. Appending to a linked list in one expression

`(l[g]=*(l[g]?&l[g]->n:&h[g])=F++)->s=p;` takes a new node from the pool, links
it after the last word of length `g` (or makes it the first word when the list
is empty), records it as the new last word and stores the word in it, all in
one line. Taking the address of either `l[g]->n` or `h[g]` with `?:` lets one
assignment write to two different places.

6. Assignment used as a loop condition

`for(w=n==N?W:h[n];s[t]=w;...)` stores the current word in the list of chosen
words and stops the loop when it is a null pointer, all in the loop test.
`if(g=k(p))` stores and tests the length in the same way.

7. Nested `if` with a dangling `else`

`if(k(w->s)) if(n==r){...} else if(t<m-1) S(...);` has two `if`s and one
`else`. The `else` belongs to the nearest `if` (`n==r`), not the first one, so
the recursion only happens when the word fits but does not use up all the
letters.

8. Search repeated with a growing word limit

`while(++m<128)S(127,n,0,h[127]);` runs the whole search again for 1 word, 2
words, 3 words and so on, printing only results with exactly `m` words. That
is why the output is sorted by word count without any sorting code. Inside
`S()`, words are tried from the longest length to the shortest, and a word of
the same length as the previous one starts at that previous word (`W`), so the
same words are never printed in a different order. `h[127]` is just a way to
write the starting list for length 127.

9. Side effects packed into expressions

`c[*p++]++,n++` uses the comma operator to count a letter and the total in one
statement, and `i=a=0` inside the printing loop also sets the exit status to
"found something". `S(n,r-n,t+1,s[t]=w)` repeats an assignment that was already
done in the loop test.

10. Implicit declaration of `printf`

There is no `#include <stdio.h>`. The compiler guesses the type of `printf`
because old C allows calling a function that was never declared.
