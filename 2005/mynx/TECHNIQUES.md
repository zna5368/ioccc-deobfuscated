# 2005/mynx

Difficulty: Medium

1. Macros that hide keywords and types

`#define I int`, `#define c char *`, `#define W while(`, `#define Q if(`,
`#define k return` and `#define G fscanf(f,` replace common words with single
letters. `Q x == '\n' || ...)` is really `if (x == '\n' || ...)`, and the
closing parenthesis looks like it has no match. `c` hides a pointer star, so
`c l[Z]` is an array of `char *` while `c *v` in `main` is `char **`. `Z` is
the buffer size 512, used for every array.

2. One letter names for everything

All globals and functions have one letter names: `f` is the open page, `w` the
terminal size, `L` the number of links, `B` the length of the line buffer `b`,
`r` the row counter, `H` the port, `l` the link table, `S` the entity table,
`T` a scratch text buffer and `h` the host name. The functions `e`, `P`, `p`,
`X`, `d` and `a` are the entity lookup, the word wrapping character printer,
the formatted printer, the pattern matcher, the block tag handler and the
attribute tag handler. Small letters and capitals mean different things
(`p` is a function, `P` another function, and in `main` `p` is also a local
pointer), which makes it easy to misread.

3. A table of `void *` mixing strings and function pointers

`Y[]` is a flat array of `void *` where even entries are tag patterns and odd
entries are the functions that handle them. `main` walks it two at a time
(`p += 2`) and calls a handler with the cast
`(* (void (*)(c )) p[1])(*p)`. `simple.c` uses an array of structs with a
`pattern` field and a typed `handler` field instead.

4. Handlers decide what to do from the pattern text

The handler is passed the pattern that matched, and looks at its first letters
to know which tag it is dealing with. In `a()`, `*s == 'a' || *s == 'f' ||
s[1] == 'f'` means "this is `<a>`, `<frame>` or `<iframe>`, so add a link",
`*s != 'a'` means "print the value", and `*s == 'b' || s[1] == 'h'` picks one
newline for `<br>`, `<hr>` and `</hN>`. `d()` checks `*s == 'h'` to put
headings at the left margin. The same function `a()` is used for links,
images and plain line breaks such as `br` (where no quoted value is found and
the text stays empty). `simple.c` gives these tests names like `is_link`.

5. A tiny wildcard matcher used as the HTML parser

There is no real HTML parsing. `X()` compares a pattern with the next bytes of
the file, ignoring case; a `*` means "skip anything until the rest matches".
If it fails, `fseek` puts the file back where it started. So `"a *href="`
matches `<a` followed later by `href=`, `"!--*-->"` skips a comment and
`"*>"` skips any other tag. The same function also skips the HTTP headers with
`X("*\r\n\r\n")`.

6. A packed entity table

`S[Z][10]` is an array of 10 byte strings where byte 0 holds the character
code and the name starts at byte 1. Reading `entities.txt` stores the name with
`S[n]+1` and the code with `*S[n] = x`; looking up checks `*S[n]` (non zero
means used) and compares `S[n]+1`. `simple.c` uses a struct with a `code` and
a `name` field that has the same layout.

7. `goto` into the middle of a loop

The local file case jumps with `goto e` over all the network code to the label
`e:` inside the `while` loop, and pressing Enter at the prompt jumps back to
`e:` to show the next screen. `simple.c` uses `if`/`else` for the two ways of
opening a page and a `do ... while` loop for paging.

8. Return values reused as lengths

`B = printf("%s", b)` stores the number of printed characters in the line
length variable just so the next line can use `b + B + 1` to find the word
that did not fit, and then `B` is set again by `snprintf` writing into the
same buffer it reads from. In `main`, `n` is used for the `%n` offset of the
path in the URL, then for the request length, then for every `recv` result.
Because of this a following link that is not an `http://` URL is requested
using the last `recv` value as the offset. `simple.c` uses separate variables
and copies the value back where the original relies on it.

9. Busy loops and expressions with side effects

`W connect(m = socket(...), ...) || errno == EINTR) ;` creates the socket
inside the argument of `connect` and keeps making new sockets with an empty
loop body until one connects. `G ungetc(fgetc(f), f) == '"' ? ... : ..., T)`
peeks at the next character inside the argument list of `fscanf` to pick the
format for a `"` or `'` quoted value.

10. Layout to look like normal code

The source is laid out with tabs and lines broken over many lines (for
example the `snprintf` and `connect` calls), and the `Y[]` table is lined up in
neat columns, so it looks tidy while the macros and names hide what it does.
