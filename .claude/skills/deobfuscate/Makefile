# inherit variables from the Makefile of the current entry because that is
# easier than trying to extract them in a shell script
# the program name can change before 2014 it was the author name after it is
# prog.c and CDEFINE is really difficult to extract because it can have things
# like this -D'g(o)'='goto o;' which prevents you from splitting by whitespace

include Makefile

.PHONY: basic compare

# create the simple.c program
basic:

# Copy the include directives from the original obfuscated file into the
# simplified source code

	-grep '#\s*include\s*<.*>' ${PROG}.c > simple.c

# Prevent the preprocessor from including anything so that it does not make the
# source file really messy

	-cpp ${PROG}.c ${CDEFINE} --no-line-commands -nostdinc >> simple.c

# Format the code so it looks nice

	clang-format -i "simple.c"

# create scripts used to verify the program works
compare:

	cp try.sh original.sh

# Get rid of paging done in the original try script

	sed -i -e 's/|\(\s\)*less.*//' "original.sh"

	cp try.sh verify.sh

# Rename the program and get rid of paging in the verify script

	sed -i -e "s/\.\/${PROG}/\.\/simple/g;s/|\(\s\)*less.*//" "verify.sh"

# build the simple program
simple: simple.c
	${CC} ${CFLAGS} $< -o $@ ${LDFLAGS}
