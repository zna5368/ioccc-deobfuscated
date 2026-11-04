/*
 * A tiny interpreter for a very small subset of Common Lisp.
 *
 * Like the original there are no #include lines; every library function is
 * implicitly declared.  The original source is also a Lisp program, which is
 * why mikeash.c (not this file) must be fed to the interpreter to get the
 * self-reproducing output.
 */

#define TOKEN_SIZE 2048
#define VARIABLE_SIZE 2048
#define SCRATCH_SIZE 12

int ch;                         /* last character read from stdin */
char scratch[SCRATCH_SIZE];     /* result of characters and arithmetic */
char int_format[] = "%d";
char token[TOKEN_SIZE];         /* the most recently read token */
int token_len;
char variable_q[VARIABLE_SIZE]; /* the one and only variable, q */

int next_char(void) { return ch = getchar(); }

int length(char *s) { return strlen(s); }

/*
 * Read one token into token[].  The character that ends a token is thrown
 * away, which is why an atom must be followed by whitespace before a ')'.
 * Strings keep their opening '"' but not the closing one.
 * Returns 0 at end of input.
 */
char *read_token(void) {
  do
    next_char();
  while (isblank(ch) || ch == '\n');

  if (ch < 0)
    return 0;

  token_len = 0;
  if (ch == ';') {
    /* comment: skip the rest of the line, then read the next token */
    while (ch != '\n')
      next_char();
    if (!read_token())
      token[token_len] = 0; /* comment at end of input gives "" */
    return token;
  }

  if (ch == '(' || ch == ')') {
    token[token_len++] = ch;
  } else if (ch == '"') {
    do
      token[token_len++] = ch;
    while (next_char() != '"');
  } else {
    do
      token[token_len++] = ch;
    while (!isblank(next_char()) && ch != '\n');
  }
  token[token_len] = 0;
  return token;
}

char *eval(void);

/* (defvar NAME VALUE): NAME is ignored, VALUE is always stored in q */
char *do_defvar(void) {
  read_token(); /* variable name */
  strcpy(variable_q, eval());
  read_token(); /* ")" */
  return variable_q;
}

/* (format DEST "...~s..." VALUE): DEST is ignored, prints to stdout */
char *do_format(void) {
  read_token(); /* destination, e.g. t */
  char *fmt = strdup(eval());
  *strchr(fmt, '~') = '%'; /* "~s" becomes "%s" */

  char *value = eval();
  int len = length(value);
  value[len] = '"'; /* the "closing quote" of a string value */
  value[len + 1] = 0;

  printf(fmt + 1, value); /* + 1 skips the opening quote */
  read_token();           /* ")" */
  return token;
}

/* (substitute NEW OLD STRING) */
char *do_substitute(void) {
  char new_char = *eval();
  char old_char = *eval();
  char *result = strdup(eval());
  for (char *p = result; *p; p++)
    if (*p == old_char)
      *p = new_char;
  read_token(); /* ")" */
  return result;
}

/* (char-upcase CHAR), only correct for lowercase letters */
char *do_char_upcase(void) {
  scratch[0] = *eval() - ('a' - 'A');
  read_token(); /* ")" */
  return scratch;
}

/* (+ A B), (- A B), (* A B), (/ A B) */
char *do_arithmetic(char op) {
  int a = atoi(eval());
  int b = atoi(eval());
  int result;

  if (op == '+')
    result = a + b;
  else if (op == '-')
    result = a - b;
  else if (op == '*')
    result = a * b;
  else
    result = a / b;

  sprintf(scratch, int_format, result);
  read_token(); /* ")" */
  return scratch;
}

/* Read and evaluate one expression.  Only the first letter of a name counts. */
char *eval(void) {
  char *tok = read_token();
  if (!tok)
    return 0;

  switch (*tok) {
  case '"': /* string literal */
    return tok;

  case '#': /* character: #\Newline or #\x */
    scratch[0] = tok[2] == 'n' ? '\n' : tok[2];
    return scratch;

  case 'q': /* the variable */
    return variable_q;

  case '(': {
    char *name = read_token();
    switch (*name) {
    case 'd':
      return do_defvar();
    case 'f':
      return do_format();
    case 's':
      return do_substitute();
    case 'c':
      return do_char_upcase();
    default:
      return do_arithmetic(*name);
    }
  }

  default: /* numbers and other atoms evaluate to themselves */
    return tok;
  }
}

int main(void) {
  while (ch != -1)
    eval();
  return 0;
}
