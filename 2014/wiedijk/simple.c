#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Untyped lambda calculus compiled by hand into C functions.
 *
 *     Y   = \f.W_f W_f
 *     W_f = \x.f(xx)
 *
 * Every value is a Word. A function value is a closure: a pointer to cells
 * where cells[0] holds the code and any later cells hold captured values.
 */
typedef union Word Word;

union Word {
    int number;
    Word *cells;
    Word (*code)(Word, ...);
};

Word make_number(int value)
{
    return (Word){.number = value};
}

/* Call a closure, handing its code the argument and the captured values. */
Word apply(Word function, Word argument)
{
    Word *closure = function.cells;

    return closure->code(argument, closure + 1);
}

/* Allocate two cells: a closure with one capture, or a delayed application. */
Word make_pair(Word first, Word second)
{
    Word *cells = malloc(2 * sizeof(Word));

    cells[0] = first;
    cells[1] = second;
    return (Word){.cells = cells};
}

/* Lazy evaluation: remember the application without performing it yet. */
Word delay(Word function, Word argument)
{
    return make_pair(function, argument);
}

/* Perform a delayed application and release its cells. */
Word force(Word delayed)
{
    Word *cells = delayed.cells;
    Word result = apply(cells[0], cells[1]);

    free(cells);
    return result;
}

/* \n. n ? n * recursion(n - 1) : 1, with recursion captured and delayed */
Word factorial_step_code(Word n, ...)
{
    va_list captures;
    Word delayed_recursion;
    Word recursion;
    Word smaller_factorial;

    va_start(captures, n);
    delayed_recursion = *va_arg(captures, Word *);
    va_end(captures);

    if (n.number == 0) {
        return make_number(1);
    }

    recursion = force(delayed_recursion);
    smaller_factorial = apply(recursion, make_number(n.number - 1));
    return make_number(n.number * smaller_factorial.number);
}

Word factorial_step(Word delayed_recursion)
{
    return make_pair((Word){.code = &factorial_step_code}, delayed_recursion);
}

/* \recursion. factorial_step(recursion), the function Y finds the fixed point of */
Word factorial_generator_code(Word delayed_recursion, ...)
{
    return factorial_step(delayed_recursion);
}

Word factorial_generator_cells[] = {{.code = &factorial_generator_code}};
Word factorial_generator = {.cells = factorial_generator_cells};

/* W_f = \x.f(xx), where xx is delayed so that it does not recurse forever */
Word self_applier_code(Word x, ...)
{
    va_list captures;
    Word function;

    va_start(captures, x);
    function = *va_arg(captures, Word *);
    va_end(captures);

    return apply(function, delay(x, x));
}

Word self_applier(Word function)
{
    return make_pair((Word){.code = &self_applier_code}, function);
}

/* Y = \f.W_f W_f */
Word y_combinator_code(Word function, ...)
{
    return apply(self_applier(function), self_applier(function));
}

Word y_combinator_cells[] = {{.code = &y_combinator_code}};
Word y_combinator = {.cells = y_combinator_cells};

int main()
{
    Word factorial = apply(y_combinator, factorial_generator);
    int seconds = apply(factorial, make_number(10)).number;

    /* 10! seconds, converted to minutes, then hours, then days */
    printf("%.0f\n", seconds / 60.0 / 60 / 24);
}
