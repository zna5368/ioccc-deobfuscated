/*
 * DDS-BASIC Interpreter (Version 1.00) - deobfuscated
 *
 * Program lines are kept as strings in an array indexed by line number.
 * Variables are single upper case letters, stored in an int array indexed
 * by the character code of the letter.
 *
 * Like the original, this file includes no headers; the Makefile adds
 * -include stdio.h and the rest of the library is implicitly declared.
 */

#define LINE_SIZE 999            /* size of buffers and of the variable table */
#define MAX_LINES (11 * LINE_SIZE) /* line numbers that can be LISTed/SAVEd */
#define END_LINE MAX_LINES         /* hidden "E"(ND) line that stops RUN */

char *malloc();

int gosub_stack[LINE_SIZE];  /* return line numbers saved by GOSUB */
int *gosub_top;              /* next free slot of gosub_stack */
int for_line[LINE_SIZE];     /* line of the FOR statement, per variable */
int for_limit[LINE_SIZE];    /* TO value of the FOR loop, per variable */
int variables[LINE_SIZE];    /* value of each variable, indexed by letter */
int line_number;             /* current line (RUN) or line being stored */
int quote_count;             /* number of '"' seen while compressing a line */

char buffer[LINE_SIZE];      /* input line / statement being executed */
char *program[12 * LINE_SIZE]; /* program text, indexed by line number */
char *parse_pos;             /* expression parser position */

FILE *file;

int expression();

/* Return the first occurrence of needle in haystack (like strstr), or 0 */
char *find(char *haystack, char *needle) {
  char *start, *h, *n;

  for (start = haystack; *start; start++) {
    for (h = start, n = needle; *n && *h == *n; h++)
      n++;
    if (n > needle && !*n)
      return start;
  }
  return 0;
}

/* Replace every two character operator op with the one character code */
void replace_operator(char *text, char *op, char code) {
  char *found;

  while ((found = find(text, op))) {
    found[0] = code;
    found[1] = ' ';
  }
}

/* Store (or delete) the numbered line that is in buffer */
void store_line() {
  char *space;

  line_number = atoi(buffer);
  if (program[line_number])
    free(program[line_number]);
  space = find(buffer, " ");
  if (space) {
    /* strlen(space) includes the space, which leaves room for the '\0' */
    program[line_number] = malloc(strlen(space));
    strcpy(program[line_number], space + 1);
  } else {
    program[line_number] = 0;
  }
}

/*
 * Expression parser: one function per precedence level.  The right hand
 * side of every operator is parsed by calling the same function again, so
 * all binary operators are right associative (8-4-2 is 8-(4-2) = 6).
 */

/* number, variable, unary minus or bracketed expression */
int primary() {
  int value;

  if (*parse_pos == '-') {
    parse_pos++;
    return -primary();
  }
  if (*parse_pos >= '0' && *parse_pos <= '9')
    return strtol(parse_pos, &parse_pos, 0); /* decimal, 0octal or 0xhex */
  if (*parse_pos == '(') {
    parse_pos++;
    value = expression();
    parse_pos++; /* skip ')' */
    return value;
  }
  return variables[*parse_pos++];
}

/* * and / */
int product() {
  int left = primary();

  switch (*parse_pos++) {
  case '*':
    return left * product();
  case '/':
    return left / product();
  default:
    parse_pos--;
    return left;
  }
}

/* + and - */
int sum() {
  int left = product();

  switch (*parse_pos++) {
  case '+':
    return left + sum();
  case '-':
    return left - sum();
  default:
    parse_pos--;
    return left;
  }
}

/* <= (stored as '$') and >= (stored as '!') */
int less_greater_equal() {
  int left = sum();

  switch (*parse_pos++) {
  case '$':
    return left <= less_greater_equal();
  case '!':
    return left >= less_greater_equal();
  default:
    parse_pos--;
    return left;
  }
}

/* < and > */
int less_greater() {
  int left = less_greater_equal();

  switch (*parse_pos++) {
  case '<':
    return left < less_greater();
  case '>':
    return left > less_greater();
  default:
    parse_pos--;
    return left;
  }
}

/* = and <> (stored as '#') */
int expression() {
  int left = less_greater();

  switch (*parse_pos++) {
  case '=':
    return left == expression();
  case '#':
    return left != expression();
  default:
    parse_pos--;
    return left;
  }
}

/* Evaluate the expression that starts at text */
int evaluate(char *text) {
  parse_pos = text;
  return expression();
}

/* Execute the statement in buffer; last points to its final character */
void execute(char *last) {
  char *keyword;
  int var;

  if (buffer[1] == '=') { /* var = exp */
    variables[buffer[0]] = evaluate(buffer + 2);
    return;
  }

  switch (buffer[0]) {
  case 'E': /* END */
    line_number = -1;
    break;
  case 'R': /* RETURN (but not REM) */
    if (buffer[2] != 'M')
      line_number = *--gosub_top;
    break;
  case 'I':
    if (buffer[1] == 'N') { /* INPUTv */
      /*
       * The input overwrites buffer, and the variable name is read
       * afterwards through last, so it survives only for short input.
       */
      fgets(buffer, LINE_SIZE, stdin);
      variables[*last] = evaluate(buffer);
    } else { /* IFexpTHENexp */
      keyword = find(buffer, "TH");
      *keyword = '\0';
      if (evaluate(buffer + 2))
        line_number = evaluate(keyword + 4) - 1;
    }
    break;
  case 'P': /* PRINT"string" or PRINTexp */
    if (buffer[5] == '"') {
      *last = '\0'; /* remove closing quote */
      puts(buffer + 6);
    } else {
      printf("%d\n", evaluate(buffer + 5));
    }
    break;
  case 'G': /* GOTOexp or GOSUBexp */
    parse_pos = buffer + 4;
    if (buffer[2] == 'S') {
      *gosub_top++ = line_number;
      parse_pos++;
    }
    line_number = expression() - 1;
    break;
  case 'F': /* FORv=expTOexp */
    keyword = find(buffer, "TO");
    *keyword = '\0';
    var = buffer[3];
    variables[var] = evaluate(buffer + 5);
    for_limit[var] = evaluate(keyword + 2);
    for_line[var] = line_number;
    break;
  case 'N': /* NEXTv */
    if (++variables[*last] <= for_limit[*last])
      line_number = for_line[*last];
    break;
  }
}

void run() {
  char *line, *out, ch[2];
  int i;

  gosub_top = gosub_stack;
  line_number = 1;
  for (i = 0; i < LINE_SIZE; i++)
    variables[i] = 0;

  while (line_number) {
    while (!(line = program[line_number]))
      line_number++;

    /* Two character operators become one character (changes the program) */
    if (!find(line, "\"")) {
      replace_operator(line, "<>", '#');
      replace_operator(line, "<=", '$');
      replace_operator(line, ">=", '!');
    }

    /* Copy the line into buffer, dropping blanks outside of strings */
    out = buffer;
    ch[1] = '\0';
    while ((ch[0] = *line)) {
      if (*line == '"')
        quote_count++;
      if (quote_count & 1 || !find(" \t", ch))
        *out++ = *line;
      line++;
    }
    *out-- = quote_count = 0;

    execute(out);
    line_number++;
  }
}

void list(FILE *out) {
  int i;

  for (i = 0; i < MAX_LINES; i++)
    if (program[i])
      fprintf(out, "%d %s\n", i, program[i]);
}

main() {
  int i;

  program[END_LINE] = "E";
  for (;;) {
    puts("Ok");
    fgets(buffer, LINE_SIZE, stdin);
    buffer[strlen(buffer) - 1] = '\0';

    switch (buffer[0]) {
    case 'R': /* RUN */
      run();
      break;
    case 'L': /* LIST */
      list(stdout);
      break;
    case 'N': /* NEW */
      for (i = 0; i < MAX_LINES; i++) {
        if (program[i])
          free(program[i]);
        program[i] = 0;
      }
      break;
    case 'B': /* BYE */
      return 0;
    case 'S': /* SAVE filename */
      file = fopen(buffer + 5, "w");
      list(file);
      fclose(file);
      break;
    case 'O': /* OLD filename */
      file = fopen(buffer + 4, "r");
      while (fgets(buffer, LINE_SIZE, file)) {
        *find(buffer, "\n") = '\0';
        store_line();
      }
      fclose(file);
      break;
    default: /* line number: store or delete a program line */
      store_line();
    }
  }
}
