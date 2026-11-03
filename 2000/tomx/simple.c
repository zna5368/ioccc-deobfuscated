#include <stdio.h>
#define true

true /*:all
#
# This file is a C program, a shell script and a Makefile at once.
#
#   C:     "true" is defined as nothing, and everything from the "/*"
#          above to the matching end-of-comment marker at the bottom is
#          one big C comment.  The compiler only sees #include and main().
#
#   shell: "#" lines are comments.  "true" ignores its argument, the
#          variables below are set, then make is run on this very file
#          and the script exits before it reaches anything else.
#
#   make:  "#" lines are comments.  The first rule above makes "true"
#          (the default goal) depend on "all", which builds $(PROG).
#

CC=cc
PROG=simple

# shell: "false" just fails quietly and the script carries on.
# make:  a rule for a target named "false" that is never built.
#        Its recipe is what the shell runs: rebuild using this file
#        ($0) as the Makefile, passing along the first argument ($1).
false :
	make -f $0 $1
	exit 0

# Only make reads past this point.

all: $(PROG)

# Pattern rule: build program "name" from the source file "name.c".
%:%.c
	$(CC) $< -o $@

clean:
	rm $(PROG)

# make: mark the targets as phony; the indented C code below is
#       treated as this rule's recipe and is never executed.
# C:    the comment ends here and main() follows.
.PHONY: /* true clean */
	int main(void)
	{
		printf("Hello, world\n");
		return 0;
	}
