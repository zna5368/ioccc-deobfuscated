#include <stdio.h>

/*
 * SORTA interpreter.
 *
 * The integer stack lives in int_stack[1..int_top] and the string stack in
 * str_stack[1..str_top] (slot 0 is unused, except as scratch space by '!').
 * Programs (macros) are stored in programs[], one per character.
 * When a program runs, its text is pushed into the pending[] buffer, which is
 * read before standard input.
 */

#define STACK_LIMIT 250 /* -Do=250 in the original Makefile */

char *str_stack[256];
static char *programs[256];
int pending[256];
int pending_count = 0;

/* replace *target with a newly allocated "*target" + "suffix" */
void append_string(char *suffix, char **target)
{
  char *result = malloc(strlen(*target) + strlen(suffix) + 1);
  sprintf(result, "%s%s", *target, suffix);
  *target = result;
}

/* return a newly allocated copy of a string */
char *copy_string(char *text)
{
  char *result = malloc(strlen(text) + 1);
  strcpy(result, text);
  return result;
}

/* read the next command character: running program text first, then stdin */
int next_char(void)
{
  if (pending_count)
    return pending[--pending_count];
  return getchar();
}

/* read characters up to the terminator into a growable string */
char *read_string(int terminator)
{
  int c;
  int size = 1;
  int length = 0;
  char *text = malloc(size);

  while ((c = next_char()) != EOF) {
    if (c == terminator)
      break;
    text[length++] = c;
    if (length == size)
      text = realloc(text, size <<= 1);
  }
  text[length] = 0;
  return text;
}

int main(int argc, char **argv)
{
  int int_stack[256];
  int int_top = 0;
  int str_top = 0;
  int in_number = 0;
  int c;

  while ((c = next_char()) != EOF) {
    if (pending_count > STACK_LIMIT || int_top > STACK_LIMIT ||
        str_top > STACK_LIMIT)
      return 2;

    /* digits build up a number on top of the integer stack */
    if (c >= '0' && c <= '9') {
      if (!in_number)
        int_stack[++int_top] = 0;
      in_number = 1;
      int_stack[int_top] = 10 * int_stack[int_top] + (c - '0');
      continue;
    }
    in_number = 0;

    switch (c) {
    case '#': /* format top integer as a string */
      if (int_top) {
        str_stack[++str_top] = malloc(13);
        sprintf(str_stack[str_top], "%d", int_stack[int_top]);
      }
      break;

    case '`': /* print top string with newline */
      if (str_top)
        puts(str_stack[str_top]);
      break;

    case '$': /* print top string without newline */
      if (str_top)
        printf("%s", str_stack[str_top]);
      break;

    case 's': /* pop n, swap top integer with the one n below it */
      if (int_top) {
        int depth = int_stack[int_top--];
        int other = int_top - depth;
        /* the slot just above the top (where n was) is the scratch space */
        if (other > 0) {
          int_stack[int_top + 1] = int_stack[other];
          int_stack[other] = int_stack[int_top];
          int_stack[int_top] = int_stack[int_top + 1];
        }
      }
      break;

    case 'S': /* pop n, swap top string with the one n below it */
      if (int_top) {
        int depth = int_stack[int_top--];
        int other = str_top - depth;
        /* the slot just above the top string is the scratch space */
        if (other > 0) {
          str_stack[str_top + 1] = str_stack[other];
          str_stack[other] = str_stack[str_top];
          str_stack[str_top] = str_stack[str_top + 1];
        }
      }
      break;

    case '[': /* push string literal [...] */
      str_stack[++str_top] = read_string(']');
      break;

    case '"': /* push string literal "..." */
      str_stack[++str_top] = read_string('"');
      break;

    case 'D': /* duplicate top integer */
      if (int_top) {
        int value = int_stack[int_top];
        int_stack[++int_top] = value;
      }
      break;

    case 'd': /* drop top integer */
      if (int_top)
        --int_top;
      break;

    case 'a': /* push argc */
      int_stack[++int_top] = argc;
      break;

    case 'l': /* pop string, push its length */
      if (str_top) {
        int_stack[++int_top] = strlen(str_stack[str_top]);
        free(str_stack[str_top--]);
      }
      break;

    case '\'': /* duplicate top string */
      if (str_top) {
        str_stack[str_top + 1] = copy_string(str_stack[str_top]);
        ++str_top;
      }
      break;

    case 'A': /* pop index, push argv[index] */
      if (int_top && int_stack[int_top] >= 0 && int_stack[int_top] < argc)
        str_stack[++str_top] = copy_string(argv[int_stack[int_top--]]);
      break;

    case 'T': /* concatenate top two strings */
      if (str_top > 1) {
        --str_top;
        append_string(str_stack[str_top + 1], &str_stack[str_top]);
      }
      break;

    case 'o': /* open(path, flags, mode) */
      if (str_top && int_top > 1) {
        int_stack[int_top - 1] = open(str_stack[str_top],
                                      int_stack[int_top - 1],
                                      int_stack[int_top]);
        --int_top;
        free(str_stack[str_top--]);
      }
      break;

    case 'O': /* close(fd) */
      if (int_top)
        close(int_stack[int_top--]);
      break;

    case 'u': /* dup(fd) */
      if (int_top)
        int_stack[int_top] = dup(int_stack[int_top]);
      break;

    case 'F': /* fork() */
      int_stack[++int_top] = fork();
      break;

    case 'w': /* wait(), push exit status or -1 */
      ++int_top;
      if (wait(&int_stack[int_top]) == -1)
        int_stack[int_top] = -1;
      break;

    case 'P': /* pipe(), push both ends */
      ++int_top;
      if (pipe(int_stack + int_top++) == -1)
        int_stack[int_top] = -1;
      break;

    case 'q': /* quit */
      return 0;

    case '=': { /* =x: pop integer, if nonzero run program x */
      char *program;
      if ((c = next_char()) == EOF)
        break;
      if (int_top && int_stack[int_top--] && (program = programs[c])) {
        int length = strlen(program);
        pending_count += length;
        if (pending_count > STACK_LIMIT || int_top > STACK_LIMIT ||
            str_top > STACK_LIMIT)
          return 2;
        /* push in reverse so the program is read front to back */
        while (*program)
          pending[--pending_count] = *program++;
        pending_count += length;
      }
      break;
    }

    case ':': /* :x: copy top string into program x */
      if (str_top) {
        if ((c = next_char()) == EOF)
          break;
        if (programs[c])
          free(programs[c]);
        programs[c] = copy_string(str_stack[str_top]);
      }
      break;

    case '!': /* execvp() top string with arguments down to an empty string */
      if (str_top) {
        int saved_top = str_top;
        str_stack[0] = str_stack[str_top];
        str_stack[str_top] = 0; /* NULL terminates argv */
        /* lop the first character off each argument until the empty one */
        while (--str_top && str_stack[str_top][0])
          str_stack[str_top]++;
        if (str_top) {
          int_stack[++int_top] =
              execvp(str_stack[0], str_stack + str_top + 1);
          str_top = saved_top;
        }
      }
      break;

    case '/': /* divide */
      if (int_top > 1) {
        int divisor = int_stack[int_top--];
        if (divisor)
          int_stack[int_top] /= divisor;
      }
      break;

    case '>': /* compare */
      if (int_top > 1) {
        int right = int_stack[int_top--];
        int_stack[int_top] = int_stack[int_top] > right;
      }
      break;

    case '+': /* add */
      if (int_top > 1) {
        int right = int_stack[int_top--];
        int_stack[int_top] += right;
      }
      break;

    case '*': /* multiply */
      if (int_top > 1) {
        int right = int_stack[int_top--];
        int_stack[int_top] *= right;
      }
      break;

    case '&': /* bitwise NAND */
      if (int_top > 1) {
        int right = int_stack[int_top--];
        int_stack[int_top] = ~(int_stack[int_top] & right);
      }
      break;

    case '_': /* negate */
      if (int_top)
        int_stack[int_top] = -int_stack[int_top];
      break;

    case ' ':
    case '\n':
      break;

    default: /* unknown command: echo it followed by '?' */
      putchar(c);
      puts("?");
    }
  }
}
