/*
 * am - Anthony's Make
 *
 * usage: am <makefile> [target]
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#ifndef RULES
#define RULES 500
#endif

/* Characters that separate words in a makefile. */
char whitespace[] = "\n\t ";

/* The processed makefile text: every logical line with macros expanded. */
char *buffer;

/* The current physical line read from the makefile. */
char line[BUFSIZ];

/*
 * Rule tables, all pointing into buffer.  Index 0 is an empty rule used
 * for names that are not the target of any rule.
 *
 *   targets[r]        "name name ..."  (the text before the ':')
 *   prerequisites[r]  "name name ..."  (the text after the ':')
 *   commands[r]       the line following the rule line.  If it starts with
 *                     a tab, it is a list of "\tcommand\0\tcommand\0..."
 *                     It is set to NULL once the commands have been run.
 */
char *targets[RULES], *prerequisites[RULES], *commands[RULES];

/*
 * Skip over the current word of length *length in *text, then skip any
 * whitespace.  *text then points at the next word and *length is set to
 * (and returned as) that word's length, which is 0 at the end of the text.
 */
static size_t next_word(char **text, size_t *length)
{
	*text += *length;
	*text += strspn(*text, whitespace);
	*length = strcspn(*text, whitespace);
	return *length;
}

/*
 * Modification time of the file named by the first word of name,
 * or 0 if the file does not exist.
 */
static time_t file_time(char *name)
{
	int status;
	char saved;
	size_t length = 0;
	struct stat info;

	next_word(&name, &length);

	/* Temporarily terminate the word so it can be passed to stat(). */
	saved = name[length];
	name[length] = '\0';
	status = stat(name, &info);
	name[length] = saved;

	return status < 0 ? 0 : info.st_mtime;
}

/*
 * Find the first rule that lists the first word of name as one of its
 * targets.  Returns the rule number, or 0 (the empty rule) if none.
 */
static int find_rule(char *name)
{
	int rule;
	char *word;
	size_t name_length = 0, word_length;

	next_word(&name, &name_length);

	for (rule = 1; targets[rule] != NULL; rule++) {
		word = targets[rule];
		word_length = 0;
		while (next_word(&word, &word_length) != 0) {
			if (word_length == name_length
			    && strncmp(name, word, name_length) == 0)
				return rule;
		}
	}
	return 0;
}

/*
 * Run each tab-indented command line in the list.  A command may be
 * prefixed by '@' (do not echo it), '-' (ignore its errors) and '+'
 * (accepted but ignored).
 */
static void run_commands(char *name, char *command)
{
	int echo, ignore_errors;

	while (*command++ == '\t') {
		echo = 1;
		ignore_errors = 0;

		while (strspn(command, "-+@") > 0) {
			if (*command == '@')
				echo = 0;
			if (*command == '-')
				ignore_errors = 1;
			command++;
		}

		if (echo)
			puts(command);

		if (system(command) != 0 && !ignore_errors) {
			fprintf(stderr, "am: Error making '%s'\n", name);
			exit(1);
		}

		/* The next command follows this one's terminating '\0'. */
		command += strlen(command) + 1;
	}
}

/*
 * Bring the target named by the first word of name up to date: first
 * update each of its prerequisites, then run its commands if the target
 * does not exist or is older than any prerequisite.
 */
static void update(char *name)
{
	time_t target_time = file_time(name);
	int out_of_date = (target_time == 0);
	int rule = find_rule(name);
	char *prerequisite = prerequisites[rule];
	char *command = commands[rule];
	size_t length = 0;

	if (next_word(&prerequisite, &length) == 0 && command != NULL
	    && *command != '\t' && out_of_date) {
		fprintf(stderr, "am: Don't know how to make '%s'.\n", name);
		exit(1);
	}

	while (length > 0) {
		update(prerequisite);
		if (target_time < file_time(prerequisite))
			out_of_date = 1;
		next_word(&prerequisite, &length);
	}

	if (out_of_date && command != NULL && *command == '\t') {
		/* Make sure every made file gets a distinct modification time. */
		sleep(1);
		run_commands(name, command);

		/* Mark the rule as done so it is not made a second time. */
		commands[rule] = NULL;
	}
}

int main(int argc, char **argv)
{
	FILE *makefile;
	struct stat info;
	size_t length;
	int rule;
	int continued;		/* previous line ended with a backslash */
	char *macro;		/* pending "NAME=value" to put in the environment */
	char *value;
	char *line_start;	/* start of the current logical line in buffer */
	char *out;		/* where the next character goes in buffer */
	char *in;		/* next character to read from line */

	if (argc < 2) {
		fprintf(stderr, "usage: am <makefile> [target]\n");
		return 2;
	}
	if (stat(argv[1], &info) != 0
	    || (makefile = fopen(argv[1], "r")) == NULL)
		return 3;

	/* Expanding macros can make the text larger than the file. */
	out = buffer = malloc(2 * info.st_size);
	if (buffer == NULL)
		return 4;

	rule = 1;
	continued = 0;
	macro = NULL;
	line_start = NULL;
	targets[0] = prerequisites[0] = commands[0] = "";

	while ((in = fgets(line, BUFSIZ, makefile)) != NULL) {
		if (rule >= RULES) {
			fprintf(stderr,
				"am: Number of target-rules exceeds %d.\n",
				RULES);
			return 1;
		}

		if (continued) {
			/* A backslash-newline and the following blanks
			 * become a single space. */
			*out++ = ' ';
			in += strspn(in, whitespace);
		} else {
			/* The line after a rule line holds its commands. */
			if (targets[rule] != NULL)
				commands[rule++] = out;

			/* A finished macro definition goes into the
			 * environment so $(NAME) and commands can see it. */
			if (macro != NULL && putenv(macro) != 0)
				fprintf(stderr,
					"am: No more environment space.\n");
			macro = NULL;

			/* Skip comments and blank lines. */
			if (line[0] == '#' || line[0] == '\n')
				continue;

			line_start = out;
		}

		while (!(continued = (in[0] == '\\' && in[1] == '\n'))
		       && *in != '\0') {
			if (*in == '$') {
				if (in[1] == '(') {
					/* $(NAME): copy in the value of the
					 * environment variable NAME. */
					char *name = in + 2;
					in += strcspn(in, ")");
					*in++ = '\0';
					value = getenv(name);
					if (value != NULL) {
						strcpy(out, value);
						out += strlen(value);
					}
					continue;
				}
				/* $$ becomes a single $. */
				if (in[1] == '$')
					++in;
			}

			if (*in == '=' && line[0] != '\t' && macro == NULL) {
				/* Macro definition "NAME = value": store it
				 * as "NAME=value" without the blanks around
				 * the '='. */
				*out = '\0';
				macro = line_start;
				length = 0;
				next_word(&macro, &length);
				out = macro + length;
				*out++ = '=';

				length = 1;
				if (next_word(&in, &length) == 0)
					*out++ = '\0';
				continue;
			}

			if (*in == ':') {
				/* Rule line "targets : prerequisites". */
				targets[rule] = line_start;
				*in = '\0';
				prerequisites[rule] = out + 1;
			}

			if (*in == '\n')
				*in = '\0';

			*out++ = *in++;
		}
	}
	fclose(makefile);

	update(argc > 2 ? argv[2] : targets[1]);
	return 0;
}
