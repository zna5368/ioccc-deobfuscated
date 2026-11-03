#include <ctype.h>
#include <stdio.h>
#include <string.h>

/*
 * A tiny Lisp interpreter. Every value - number, symbol, list or function -
 * is a C string: a list is simply its printed text, e.g. "(a b c)". Lists are
 * taken apart by counting parentheses in that text.
 *
 * Like the original there is no error checking: bad input will crash, and
 * there must be no whitespace directly after an opening parenthesis.
 */

#define STRING_SIZE 199  /* every new string gets this many bytes */
#define MAX_BINDINGS 103 /* size of the symbol table */
#define BUILTIN_COUNT 14 /* number of predefined symbols */

char *sbrk();
double atof();

/* A built-in function receives its argument list unevaluated */
typedef char *(*builtin)(char *args);

/*
 * Symbol table, searched from the newest entry to the oldest, so later
 * bindings (function parameters, defuns) hide earlier ones. The value of a
 * user function is the text "((params) body)", the value of a built-in is
 * a pointer to its C function stored as a char *.
 */
struct binding {
  char *name;
  char *value;
};

char *eval(char *expr);
char *builtin_function(char *args);
char *builtin_quote(char *args);
char *builtin_lambda(char *args);
char *builtin_defun(char *args);
char *builtin_if(char *args);
char *builtin_equal(char *args);
char *builtin_less(char *args);
char *builtin_add(char *args);
char *builtin_subtract(char *args);
char *builtin_multiply(char *args);
char *builtin_car(char *args);
char *builtin_cdr(char *args);
char *builtin_cons(char *args);

struct binding table[MAX_BINDINGS] = {
    {"function", (char *)builtin_function},
    {"quote", (char *)builtin_quote},
    {"lambda", (char *)builtin_lambda},
    {"defun", (char *)builtin_defun},
    {"if", (char *)builtin_if},
    {"equal", (char *)builtin_equal},
    {"<", (char *)builtin_less},
    {"+", (char *)builtin_add},
    {"-", (char *)builtin_subtract},
    {"*", (char *)builtin_multiply},
    {"car", (char *)builtin_car},
    {"cdr", (char *)builtin_cdr},
    {"cons", (char *)builtin_cons},
    {"t", "t"},
};
int table_size = BUILTIN_COUNT;

/* Memory is taken from the system and never given back */
char *new_string(void) { return sbrk(STRING_SIZE); }

int is_nil(char *value) { return strcmp(value, "()") == 0; }

/*
 * Length of the first element of text, not counting leading whitespace.
 * An element is either an atom, which ends at whitespace or a parenthesis,
 * or a whole parenthesized list.
 */
int element_length(char *text) {
  int depth = 0;
  char *end;

  while (isspace(*text))
    text++;
  end = text;
  while (depth != 0 ||
         !(isspace(*end) || *end == ')' || (*end == '(' && end != text))) {
    depth += (*end == '(') - (*end == ')');
    end++;
  }
  return end - text;
}

/* The first element of a list, as a new string */
char *first(char *list) {
  char *element;
  int length;

  list++; /* skip '(' */
  length = element_length(list);
  element = strncpy(new_string(), list, length);
  element[length] = '\0';
  return element;
}

/* A new list holding everything after the first element */
char *rest(char *list) {
  char *result;

  list++; /* skip '(' */
  list += element_length(list);
  while (isspace(*list))
    list++;
  result = new_string();
  result[0] = '(';
  strcpy(result + 1, list);
  return result;
}

/* Value of the first and second arguments in an argument list */
char *first_arg(char *args) { return eval(first(args)); }
char *second_arg(char *args) { return eval(first(rest(args))); }

char *number_to_string(double number) {
  char *result = new_string();

  sprintf(result, "%lf", number);
  return result;
}

/* (function f) - the function value of f */
char *builtin_function(char *args) { return first_arg(args); }

/* (quote x) - x itself, not evaluated */
char *builtin_quote(char *args) { return first(args); }

/* (lambda (params) body) - the function is its own text "((params) body)" */
char *builtin_lambda(char *args) { return args; }

/* (defun name (params) body) - add a permanent binding, return the name */
char *builtin_defun(char *args) {
  table[table_size].value = rest(args);
  table[table_size].name = first(args);
  return table[table_size++].name;
}

/* (if condition then else) - "()" is false, anything else is true */
char *builtin_if(char *args) {
  if (is_nil(first_arg(args)))
    args = rest(args); /* skip over the "then" part */
  return eval(first(rest(args)));
}

/* Numbers are compared by value, everything else by text */
int is_different(char *x, char *y) {
  return isdigit(*x) ? atof(x) != atof(y) : strcmp(x, y);
}

/* (equal x y) */
char *builtin_equal(char *args) {
  char *x = first_arg(args);
  char *y = second_arg(args);

  return is_different(x, y) ? "()" : "t";
}

/* (< x y) */
char *builtin_less(char *args) {
  double x = atof(first_arg(args));
  double y = atof(second_arg(args));

  return x < y ? "t" : "()";
}

/* (+ x y) */
char *builtin_add(char *args) {
  double x = atof(first_arg(args));
  double y = atof(second_arg(args));

  return number_to_string(x + y);
}

/* (- x y) */
char *builtin_subtract(char *args) {
  double x = atof(first_arg(args));
  double y = atof(second_arg(args));

  return number_to_string(x - y);
}

/* (* x y) */
char *builtin_multiply(char *args) {
  double x = atof(first_arg(args));
  double y = atof(second_arg(args));

  return number_to_string(x * y);
}

/* (car list) */
char *builtin_car(char *args) { return first(first_arg(args)); }

/* (cdr list) */
char *builtin_cdr(char *args) { return rest(first_arg(args)); }

/* (cons x list) - glue "(x" onto the front of the list text */
char *builtin_cons(char *args) {
  char *head = first_arg(args);
  char *tail = second_arg(args);
  char *result = new_string();

  if (is_nil(tail))
    sprintf(result, "(%s)", head);
  else
    sprintf(result, "(%s %s", head, tail + 1); /* tail + 1 skips its '(' */
  return result;
}

char *eval(char *expr) {
  char *function, *args, *params, *value, *result;
  int i, saved_size, param_slot;

  /* numbers and the empty list evaluate to themselves */
  if (isdigit(*expr) || is_nil(expr))
    return expr;

  /* symbols evaluate to their newest binding */
  for (i = table_size; i--;)
    if (strcmp(expr, table[i].name) == 0)
      return table[i].value;

  /* anything else is a function call (function arg ...) */
  function = eval(first(expr));
  args = rest(expr);

  /* a built-in is a C function pointer; it gets the unevaluated arguments */
  if (*function != '(')
    return ((builtin)function)(args);

  /*
   * A user function "((params) body)". Evaluate each argument into a new,
   * still unnamed, table entry, then name those entries after the
   * parameters, run the body and throw the entries away again.
   */
  saved_size = param_slot = table_size;
  while (!is_nil(args)) {
    value = eval(first(args));
    table[table_size].value = value;
    table[table_size].name = "";
    table_size++;
    args = rest(args);
  }
  for (params = first(function); !is_nil(params); params = rest(params))
    table[param_slot++].name = first(params);

  result = eval(first(rest(function)));
  table_size = saved_size;
  return result;
}

int main() {
  char *input, *end;
  int ch, depth;

  for (;;) {
    /* read one expression: stop at whitespace outside of any parentheses */
    input = end = new_string();
    depth = 0;
    for (;;) {
      ch = getchar();
      if (depth == 0 && isspace(ch))
        break;
      if (ch == EOF)
        exit(0);
      depth += (ch == '(') - (ch == ')');
      *end++ = ch;
    }
    *end = '\0';

    puts(eval(input));
  }
}
