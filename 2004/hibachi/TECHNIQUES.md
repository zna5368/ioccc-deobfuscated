# 2004/hibachi

Difficulty: Medium

1. Keyword and function macros

The first lines define `B` as `break;`, `Q` as `if(`, `R` as `return`, `V` as
`getenv(`, `S` as `snprintf(` and `C` as `strcspn(u,`. The code then reads
`Q (v = V "SERVER_PORT")) == 0)`, which only makes sense once you replace the
letters. Some macros open a parenthesis that the code closes later, so the
brackets in the source do not seem to match up.

2. String literal macro

`T` is `"DOCUMENT_ROOT"`. In `S w, Z, T "=%s/%s", V T), l)` the macro sits next
to another string, and C joins neighbouring string literals together. So the
format string is really `"DOCUMENT_ROOT=%s/%s"` and the second `T` is
`getenv("DOCUMENT_ROOT")`.

3. Short, meaningless names

Global buffers are named `r`, `q`, `l` and `w`, the error messages are `m` and
`e`, and the two helper functions are `X` (send a response) and `A` (copy a
header into the environment). `Z` hides the buffer size 8192. In simple.c
these became `request`, `response`, `not_found`, `send_response` and so on.

4. Reusing variables for different jobs

`f` is first the listening socket, then the count of newlines while reading the
request, and finally the file being sent. `i` is the port number, the address
length, the request length and a string position. `l` holds the host name and
is then reused for `SCRIPT_FILENAME=...`, and `q` holds both the
`QUERY_STRING=...` variable and the response headers. You have to follow the
code line by line to know what a variable means at any moment.

5. Magic offsets into strings

`w+14` and `l+16` skip past `"DOCUMENT_ROOT="` (14 characters) and
`"SCRIPT_FILENAME="` (16 characters) to get the path. `r + 4 + (*r != 'G')`
skips `"GET "` or the one-letter-longer `"HEAD "` and `"POST "`. In `A()`,
`i + 5` drops the first five letters of names like `"HTTP_COOKIE"` and
`"CONTENT_LENGTH"`, so it searches the headers for `"COOKIE"` and
`"NT_LENGTH"`. The method is tested with single letters: `*r != 'H'` means
"not HEAD", and `r[2] != 'S'` is how POST is spotted.

6. Writing into getenv() results

`getenv()` normally only reads a variable. Here,
`sscanf(r, "%4s", V "REQUEST_METHOD"))` writes the method straight into the
memory that `getenv()` returns. This works because `hibachi-start.sh` sets
`REQUEST_METHOD`, `REMOTE_ADDR`, `HTTP_COOKIE` and others to long dummy values
first, which leaves room for the real values. This is the "`getenv()` ==
`putenv()`" joke in the README.

7. Header names rewritten while reading

The loop that reads the request one byte at a time also counts newlines in `f`
and sets `f` back to 0 at each `:`. While `f` is not 0 the character is part
of a header name, so it is upper-cased and `-` becomes `_`. One loop reads the
request, finds the blank line at the end and turns `Content-Type:` into
`CONTENT_TYPE:` so it can be matched against environment variable names.

8. A ternary inside a format call

`X()` uses `j ? j : C "")` and `j ? "" : u` inside one `snprintf`. When the
length is 0, the status text (for example `"404 Not Found"`) is also used as
the body, and its length comes from `strcspn(u, "")`, which is just a
roundabout `strlen(u)`. simple.c uses an `if`/`else` with `strlen()` instead.

9. Using break to finish a request

The whole server is one `for ( ; ; close(t))` loop. The parent uses `continue`
after `fork()`, and `close(t)` sits in the loop's increment part. The child
handles the request inside the same loop and uses `break` (the `B` macro) at
every exit point to jump to the `shutdown()` after the loop. simple.c moves
the request code into `handle_request()` and uses `return` instead.

10. Misleading formatting

The code is indented with four tabs per level and the `snprintf` call in `X()`
is broken across lines in odd places, which hides how short each statement
is. A stray `R 1;` just after the declarations in `main` makes the program
exit straight away, so nothing after it ever runs. This line is not in
`hibachi.orig.c`.
