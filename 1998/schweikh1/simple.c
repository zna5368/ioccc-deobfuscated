/*
 * Standard header conformance test.
 *
 * The original source tried to include its headers through the token pasting
 * macro H(x) <st%:%:x##.h>, but those lines are commented out and the headers
 * are supplied by the Makefile with -include instead.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INFO_FILE "info"
#define TEMP_FILE "r"
#define PREPROCESS_COMMAND "cc -ansi -E -dM -undef %s /usr/include/%s>" TEMP_FILE

/*
 * The author meant to cut each "#define NAME value" line after NAME by
 * stopping at a space or '(' (the characters hidden after a \0 inside a
 * #line file name).  gcc drops everything after the \0 in __FILE__, so the
 * original actually points just past the end of the string where it finds an
 * empty set of delimiters.  The whole rest of the line is therefore used as
 * the name, which is reproduced here so the output stays identical.
 */
#define NAME_DELIMITERS ""

#define MAX_STANDARDS 10
#define MAX_IDENTIFIERS (11 << 11)
#define IDENTIFIER_SIZE 33
#define LINE_SIZE (40 << 10)

/*
 * Check whether a macro name is allowed by one of the patterns.
 * A pattern "*abc" matches names ending in "abc", a pattern "abc*" (any other
 * pattern containing '*') matches names starting with "abc", and anything
 * else must match exactly.
 */
static int matches_any(char patterns[][IDENTIFIER_SIZE], int count,
                       char *name)
{
	int i;

	for (i = 0; i < count; ++i) {
		char *pattern = patterns[i];
		size_t length = strlen(pattern);

		if (strchr(pattern, '*')) {
			if (*pattern == '*') {
				if (strcmp(pattern + 1, name + strlen(name) + 1 - length) == 0)
					return 1;
			} else {
				if (strncmp(pattern, name, length - 1) == 0)
					return 1;
			}
		} else if (strcmp(pattern, name) == 0) {
			return 1;
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	FILE *macros;
	int count, always_count, header_count, status;
	int standard = argc < 2 ? '0' : *argv[1];

	char standards[MAX_STANDARDS][100];
	char command[400];
	char line[LINE_SIZE];
	char always_reserved[MAX_IDENTIFIERS][IDENTIFIER_SIZE];
	char header_names[MAX_IDENTIFIERS][IDENTIFIER_SIZE];
	char which[11];

	if (freopen(INFO_FILE, "r", stdin) == NULL)
		return 0;

	/* Paragraph 1: "<digit> <cpp options>" for each standard. */
	count = 0;
	for (;;) {
		if (!fgets(standards[count], (int)sizeof *standards, stdin))
			return 1;
		if (standards[count][0] == '#')
			continue;
		if (standards[count][0] == '\n')
			break;
		standards[count][strlen(standards[count]) - 1] = '\0';
		++count;
	}

	/* Paragraph 2: identifiers that are reserved regardless of header. */
	always_count = 0;
	for (;;) {
		do {
			if (!fgets(line, (int)sizeof line, stdin))
				return 1;
		} while (line[0] == '#');

		if (sscanf(line, "%s%s", which, always_reserved[always_count]) != 2)
			break;
		if (strchr(which, standard))
			++always_count;
	}

	/* Paragraph 3: identifiers allowed by a header, followed by the header. */
	header_count = 0;
	while (fgets(line, (int)sizeof line, stdin) != NULL) {
		char standards_list[11], header_marker;

		if (line[0] == '#')
			continue;

		switch (sscanf(line, "%s%s %c", standards_list,
		               header_names[header_count], &header_marker)) {
		case 2:
			/* an identifier of the upcoming header */
			if (strchr(standards_list, standard))
				++header_count;
			break;

		case 3:
			/* the header line itself: test what the header defines */
			if (strchr(standards_list, standard)) {
				char *name = line + strlen("#define ");
				char *header = header_names[header_count];

				printf("<%s>:\n", header);
				sprintf(command, PREPROCESS_COMMAND,
				        standards[standard - '0'] + 2, header);
				status = system(command);
				(void)status;

				macros = fopen(TEMP_FILE, "r");
				if (macros) {
					while (fgets(line, (int)sizeof line, macros) != NULL) {
						name[strcspn(name, NAME_DELIMITERS)] = '\0';
						if (!matches_any(header_names, header_count, name) &&
						    !matches_any(always_reserved, always_count, name))
							puts(line);
					}
				}
				if (macros != NULL)
					header_count = fclose(macros);
			}
			break;
		}
	}
	return remove(TEMP_FILE);
}
