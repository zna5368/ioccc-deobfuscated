# 1993/plummer

Difficulty: Easy

1. Confusing variable names

The global pointers are named `_` and `O`, and `main()` uses `S` for `argc`
and `l` for `argv`. Each one looks like something else: `O` like the digit
zero, `l` like the digit one, `S` like a five, and `_` is easy to miss
completely. An expression like `*(O=*(l+++S-1)-1)` is hard to read because
your eyes keep reading letters as numbers.

2. Old K&R style function definition

`main(S,l)char**l;{...}` declares the parameter types after the parameter
list, and `S` has no type at all, so it defaults to `int`. This old style is
no longer common, so it is harder to see that `S` is `argc` and `l` is
`argv`.

3. Character constants computed from argc and the carriage return

The digits `'9'` and `'0'` never appear. Instead the code writes
`(S+*O+S)*S` and `(S+*O)*S`. `*O` holds a carriage return (13) and `S` is 3
when the program gets its two required arguments, so these work out to
`(3+13+3)*3 = 57` (`'9'`) and `(3+13)*3 = 48` (`'0'`). The carriage return
itself is written as the plain number `13` instead of `'\r'`.

4. Relying on argv strings being stored next to each other

Instead of calling `strlen()` to find the end of the number, the program
assumes that the second argument starts right after the first argument's
NUL byte in memory. So `argv[2] - 1` points at the end of `argv[1]`. That
byte is turned into a `'\r'`, and the first byte of `argv[2]` is set to `0`
to end the string again. Now printing `argv[1]` also prints a carriage
return, which moves the cursor back so the next number overwrites it.

5. Combined pointer increment and indexing

`*(l+++S-1)` mixes a post-increment `l++` with addition. It reads the last
argument using the old value of `l`, while also moving `l` forward so that
`*l` becomes `argv[1]` for the rest of the program. `*l[1]` then means
`argv[2][0]`.

6. Comma operator to join statements

`*(O=...)=13,*l[1]=0;` uses a comma to put two assignments into one
statement, hiding the second assignment at the end of a long line.

7. Whole algorithm packed into a for loop header

The inner loop `for(printf(*l),_=O-1;_>=*l&&++*_>(S+*O+S)*S;*_--=(S+*O)*S);`
has an empty body. The print happens in the start part, the digit increment
happens inside the loop condition (`++*_`), and the carry (set the digit to
`'0'` and move left) happens in the step part. The loop only keeps going
while a digit rolls past `'9'`, so it acts like adding one to the number.
The outer `for(;;)` repeats this forever.

8. User input used as a printf format string

`printf(*l)` passes the user's argument directly as the format. It works for
digits, but it hides that the program is just printing a string, and it
would misbehave if the argument contained a `%`.
