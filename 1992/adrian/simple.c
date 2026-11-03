#include <stdio.h>
#include <string.h>

/*
 * adgrep - a grep driven by a deterministic finite automaton (DFA).
 *
 * The DFA specification file contains lines of the form:
 *     <state> chars1 <dest1> chars2 <dest2> ...
 * A state written as [name] is an accepting state.
 */

#define MAX_STATES 257
#define MAX_RULES 17
#define NAME_LEN 8
#define LINE_LEN 999

struct rule {
  char chars[NAME_LEN + 1]; /* characters that trigger this rule */
  int dest;                 /* index of the destination state */
};

struct state {
  char name[NAME_LEN + 1]; /* "<name>" or "[name]" */
  char accepting;          /* true if the name contains ']' */
  int num_rules;
  struct rule rules[MAX_RULES];
};

struct state states[MAX_STATES];
int num_states = 0;

char *whitespace = " \t\n";

void error(int code, int line_number, char *detail) {
  static char *messages[] = {"too_many_states", "too_many_rules",
                             "no_destination"};

  fputs(messages[code], stderr);
  fprintf(stderr, "(%d): %s\n", line_number, detail);
  exit(1);
}

/* copy at most NAME_LEN characters and always terminate the string */
void copy_name(char *dest, char *src) {
  strncpy(dest, src, NAME_LEN);
  dest[NAME_LEN] = 0;
}

/* find a state by name, creating it if it does not exist yet */
int find_state(int line_number, char *name) {
  int i;

  if (strlen(name) > NAME_LEN)
    name[NAME_LEN] = 0;

  for (i = 0; i < num_states; i++)
    if (!strcmp(states[i].name, name))
      return i;

  if (num_states == MAX_STATES)
    error(0, line_number, name);

  copy_name(states[num_states].name, name);
  states[num_states].accepting = strchr(name, ']') != 0;
  states[num_states].num_rules = 0;
  return num_states++;
}

/* run the automaton over a string; return true if it ends in an accepting
 * state */
int accepts(char *text) {
  int current = 0;
  int i;

  for (; *text; text++) {
    for (i = 0;; i++) {
      struct rule *r;

      if (i == states[current].num_rules)
        return 0; /* no rule matches: reject */

      r = &states[current].rules[i];
      if (r->chars[0] == '.' || strchr(r->chars, *text)) {
        current = r->dest;
        break;
      }
    }
  }
  return states[current].accepting;
}

int main(int argc, char *argv[]) {
  char line[LINE_LEN], *token;
  FILE *spec;
  int line_number = 0;
  int current;

  spec = fopen(argc >= 2 ? argv[1] : "adrian.c", "rt");
  if (!spec)
    exit(1);

  /* read the automaton specification */
  while (fgets(line, LINE_LEN - 1, spec)) {
    line_number++;

    token = strtok(line, whitespace);
    if (!token)
      continue; /* blank line */

    current = find_state(line_number, token);

    while ((token = strtok(0, whitespace))) {
      struct state *st = &states[current];

      if (st->num_rules == MAX_RULES)
        error(1, line_number, st->name);

      copy_name(st->rules[st->num_rules].chars, token);

      if (!(token = strtok(0, whitespace)))
        error(2, line_number, st->rules[st->num_rules].chars);

      st->rules[st->num_rules].dest = find_state(line_number, token);
      st->num_rules++;
    }
  }

  /* print every line of standard input accepted by the automaton */
  while (fgets(line, LINE_LEN - 1, stdin)) {
    line[strlen(line) - 1] = '\0';
    if (accepts(line))
      puts(line);
  }
  return 0;
}
