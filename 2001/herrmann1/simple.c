/*
 * 2001/herrmann1 - a Turing machine that runs inside the C preprocessor.
 *
 * Compiled directly (no -Dprg=...) this is an ordinary C program that prints
 * the usage message.
 *
 * Run through simple.sh 'prg=file.turing' ['tape="O I O"'], the file is fed
 * to "cc -E" over and over again.  Each pass reads the machine's state from
 * the #define lines at the top of the file, prints one animation frame as a
 * comment, and writes a new file containing:
 *
 *     the next state as #define lines
 *     the body below with real '#' directives    (the code that runs)
 *     the body below again in quoted form         (data used to reprint it)
 *
 * When the machine halts (or reaches a state that has no #define) the pass
 * writes a normal C program instead that prints the final tape.  Nothing
 * changes after that, so simple.sh stops and compiles it.
 *
 * Quoted form: every directive of the body is written as "QUOTED(#,) ...",
 * and every directive that the body prints (the new state) is written as
 * "QUOTED(QUOTED(#,),) ...".  QUOTED has two meanings:
 *
 *     #define QUOTED(text, nothing) text ## nothing      -- unquote
 *     #define QUOTED(text, nothing) QUOTED(text, nothing) -- keep as is
 *
 * The first one strips exactly one QUOTED layer: an operand of ## is not
 * macro expanded, and QUOTED cannot expand again inside its own result.
 * So the body printed with the first meaning becomes real directives
 * ("QUOTED(#,) define" -> "# define"), and printed with the second
 * meaning it comes out unchanged.
 */

#ifndef prg

#include <stdio.h>

int main(void)
{
    printf("Usage: prg=myprg.turing [tape=\"O I O...\"]\n\n");
    return 2;
}

#elif !defined BOOTSTRAP_DONE

/* ---- pass 1: write the starting state and unpack the body ---- */

/* -Dprg=herrmann1.times2  ->  "herrmann1.times2" */
#define FILE_NAME_STRING_(name) #name
#define FILE_NAME_STRING(name) FILE_NAME_STRING_(name)
#define PROGRAM_FILE FILE_NAME_STRING(prg)

/* The tape comes from -Dtape=..., else from the program's own
 * "#define tape ...", else it is a single blank cell (O). */
#ifndef tape
#include PROGRAM_FILE
#endif
#ifndef tape
#define tape O
#endif

#define HASH #
#define BACKSLASH \ /* a lone backslash: continues a printed line */

/* Left of the head: nothing.  Under the head: a blank. */
HASH define LEFT_A LEFT_EDGE
HASH define CELL_A O

/* Right of the head: the whole tape as a nested list
 *     RIGHT_CELL(O, RIGHT_CELL(I, ... RIGHT_EDGE ,) ,)
 * The opening halves come from expanding "tape" with O and I as
 * "RIGHT_CELL(O," and "RIGHT_CELL(I,", the closing halves from expanding
 * it again with both as ",)".  The printed backslash joins the two lines. */
#define O RIGHT_CELL(O,
#define I RIGHT_CELL(I,
HASH define RIGHT_A tape BACKSLASH
#undef O
#undef I
#define O ,)
#define I ,)
RIGHT_EDGE tape
#undef O
#undef I
#undef tape

/* A made up first move: write O, step right, enter state "start". */
HASH define TRANSITION_A O, right, start
HASH define CHECK(O, right, start) O*I + 2*HALT
HASH define STATUS CHECK(0, right, start)
HASH define STATE_LABEL Initializing...

#undef HASH
#undef BACKSLASH

/* Print the body twice: once unquoted (by including this file again) and
 * once quoted (below, after the include returns). */
#define QUOTED(text, nothing) text ## nothing
#define BOOTSTRAP_DONE
#include __FILE__
#undef QUOTED
#define QUOTED(text, nothing) QUOTED(text, nothing)

#endif

#ifdef prg

/* so that the "include __FILE__" line below is printed as it is */
#undef __FILE__

/* ======================= the body (quoted form) ======================= */

QUOTED(#,) ifndef SKIP_BODY
QUOTED(# define QUOTED(text, nothing) text ## nothing,)

/* --- The state:
 *       LEFT    cells left of the head, nearest first:
 *               LEFT_CELL(I, LEFT_CELL(O, LEFT_EDGE ,) ,)
 *       CELL    the symbol under the head (O or I)
 *       RIGHT   cells right of the head, nearest first:
 *               RIGHT_CELL(I, RIGHT_CELL(I, RIGHT_EDGE ,) ,)
 *       TRANSITION  what to do now, e.g. "I, left, st3"
 *       CHECK/STATUS  running, halted or undefined state (see below)
 *       STATE_LABEL   state name plus a period, e.g. "st3."
 *     A #define cannot use its own old value, so the state is double
 *     buffered: a pass reads the _A macros and writes the _B macros, or
 *     the other way round. --- */
QUOTED(#,) ifdef TRANSITION_A
QUOTED(#,) define OLD_LEFT LEFT_A
QUOTED(#,) define OLD_CELL CELL_A
QUOTED(#,) define OLD_RIGHT RIGHT_A
QUOTED(#,) define OLD_TRANSITION TRANSITION_A
QUOTED(#,) define NEW_LEFT LEFT_B
QUOTED(#,) define NEW_CELL CELL_B
QUOTED(#,) define NEW_RIGHT RIGHT_B
QUOTED(#,) define NEW_TRANSITION TRANSITION_B
QUOTED(#,) else
QUOTED(#,) define OLD_LEFT LEFT_B
QUOTED(#,) define OLD_CELL CELL_B
QUOTED(#,) define OLD_RIGHT RIGHT_B
QUOTED(#,) define OLD_TRANSITION TRANSITION_B
QUOTED(#,) define NEW_LEFT LEFT_A
QUOTED(#,) define NEW_CELL CELL_A
QUOTED(#,) define NEW_RIGHT RIGHT_A
QUOTED(#,) define NEW_TRANSITION TRANSITION_A
QUOTED(#,) endif

/* --- Status.  CHECK's parameters are the transition that was looked up
 *     (e.g. "I, left, st3"), "HALT" for the stop state, or one undefined
 *     name such as "st5_I".  STATUS calls it with 0 in the place of O, I
 *     or HALT, so with O = I = HALT = 1 the body O*I + 2*HALT gives
 *     2 = running, 1 = halted, 3 = undefined state. --- */
QUOTED(#,) define HALTED 1
QUOTED(#,) define UNDEFINED 3
QUOTED(#,) define O 1
QUOTED(#,) define I 1
QUOTED(#,) define HALT 1
QUOTED(#,) if STATUS == UNDEFINED
QUOTED(#,) define MACHINE_STOPPED
QUOTED(#,) define BAD_STATE
QUOTED(#,) elif STATUS == HALTED
QUOTED(#,) define MACHINE_STOPPED
QUOTED(#,) endif
QUOTED(#,) undef HALTED
QUOTED(#,) undef UNDEFINED
QUOTED(#,) undef O
QUOTED(#,) undef I
QUOTED(#,) undef HALT
QUOTED(#,) undef CHECK
QUOTED(#,) undef STATUS

/* --- Start of the output: the finished C program, or a frame comment --- */
QUOTED(#,) ifdef MACHINE_STOPPED
QUOTED(#,) define LINE "\n"
QUOTED(#,) define HEAD "*"
QUOTED(#,) define ABOVE " "
QUOTED(#,) define BELOW " "
QUOTED(#,) define ELLIPSIS ".."
QUOTED(#,) define O "O"
QUOTED(#,) define I "I"
QUOTED(#,) define STRINGIFY_(x) #x
QUOTED(#,) define STRINGIFY(x) STRINGIFY_(x)
QUOTED(QUOTED(#,),) include <stdio.h>
int main(void)
{
printf(
QUOTED(#,) ifdef BAD_STATE
"Undefined state " STRINGIFY(STATE_LABEL) "\n\nTape:"
QUOTED(#,) else
"Final tape:"
QUOTED(#,) endif
QUOTED(#,) undef STRINGIFY_
QUOTED(#,) undef STRINGIFY
QUOTED(#,) else
QUOTED(#,) define LINE *
QUOTED(#,) define HEAD *
QUOTED(#,) define ABOVE _
QUOTED(#,) define BELOW ~
QUOTED(#,) define ELLIPSIS ...
/* "/ *" is printed, and simple.sh's sed turns it into the "/" "*" that
 * opens the frame comment; the next pass throws the comment away. */
QUOTED(#,) define COMMENT_OPEN / *
COMMENT_OPEN
*
* State: STATE_LABEL
QUOTED(#,) undef COMMENT_OPEN
QUOTED(#,) endif

/* --- Draw the tape: three rows, each made by giving the list macros a
 *     different meaning and expanding the two halves of the tape.  The
 *     left half is drawn innermost first, so it comes out left to right. --- */
LINE
QUOTED(#,) define BLANK ABOVE
QUOTED(#,) define LEFT_EDGE BLANK BLANK BLANK BLANK
QUOTED(#,) define RIGHT_EDGE BLANK BLANK BLANK BLANK
QUOTED(#,) define LEFT_CELL(symbol, rest, nothing) rest BLANK
QUOTED(#,) define RIGHT_CELL(symbol, rest, nothing) BLANK rest
LINE OLD_LEFT HEAD OLD_RIGHT
QUOTED(#,) undef LEFT_EDGE
QUOTED(#,) undef RIGHT_EDGE
QUOTED(#,) undef LEFT_CELL
QUOTED(#,) undef RIGHT_CELL
QUOTED(#,) define LEFT_EDGE ELLIPSIS O O
QUOTED(#,) define RIGHT_EDGE O O ELLIPSIS
QUOTED(#,) define LEFT_CELL(symbol, rest, nothing) rest symbol
QUOTED(#,) define RIGHT_CELL(symbol, rest, nothing) symbol rest
LINE OLD_LEFT OLD_CELL OLD_RIGHT
QUOTED(#,) undef LEFT_EDGE
QUOTED(#,) undef RIGHT_EDGE
QUOTED(#,) undef LEFT_CELL
QUOTED(#,) undef RIGHT_CELL
QUOTED(#,) undef BLANK
QUOTED(#,) define BLANK BELOW
QUOTED(#,) define LEFT_EDGE BLANK BLANK BLANK BLANK
QUOTED(#,) define RIGHT_EDGE BLANK BLANK BLANK BLANK
QUOTED(#,) define LEFT_CELL(symbol, rest, nothing) rest BLANK
QUOTED(#,) define RIGHT_CELL(symbol, rest, nothing) BLANK rest
LINE OLD_LEFT HEAD OLD_RIGHT
QUOTED(#,) undef LEFT_EDGE
QUOTED(#,) undef RIGHT_EDGE
QUOTED(#,) undef LEFT_CELL
QUOTED(#,) undef RIGHT_CELL
QUOTED(#,) undef BLANK
LINE
QUOTED(#,) undef LINE
QUOTED(#,) undef HEAD
QUOTED(#,) undef ABOVE
QUOTED(#,) undef BELOW
QUOTED(#,) undef ELLIPSIS
QUOTED(#,) undef O
QUOTED(#,) undef I

QUOTED(#,) ifdef MACHINE_STOPPED

/* --- End of the finished C program.  Its directive copy of the body that
 *     follows is switched off by SKIP_BODY. --- */
);
QUOTED(#,) ifdef BAD_STATE
return 1;
QUOTED(#,) else
return 0;
QUOTED(#,) endif
}
QUOTED(QUOTED(#,),) define SKIP_BODY

QUOTED(#,) else

*
*/

/* --- One step: write the symbol, move the head --- */
QUOTED(#,) define FIRST_(a, b, c) a
QUOTED(#,) define SECOND_(a, b, c) b
QUOTED(#,) define THIRD_(a, b, c) c
QUOTED(#,) define SYMBOL_TO_WRITE(transition) FIRST_(transition)
QUOTED(#,) define DIRECTION(transition) SECOND_(transition)
QUOTED(#,) define NEXT_STATE(transition) THIRD_(transition)

QUOTED(#,) define left 0
QUOTED(#,) define right 1
QUOTED(#,) if DIRECTION(OLD_TRANSITION) == right
QUOTED(#,) define MOVING_RIGHT
QUOTED(#,) endif
QUOTED(#,) undef left
QUOTED(#,) undef right

/* push: LEFT_CELL/RIGHT_CELL are undefined, so this is plain text.
 * pop:  "nothing ## rest" gives back the rest of the list unexpanded. */
QUOTED(#,) ifdef MOVING_RIGHT
QUOTED(QUOTED(#,),) define NEW_LEFT LEFT_CELL(SYMBOL_TO_WRITE(OLD_TRANSITION), OLD_LEFT, )
QUOTED(#,) define RIGHT_CELL(symbol, rest, nothing) nothing ## rest
QUOTED(QUOTED(#,),) define NEW_RIGHT OLD_RIGHT
QUOTED(#,) undef RIGHT_CELL
QUOTED(#,) define ENTERED_STACK OLD_RIGHT
QUOTED(#,) else
QUOTED(QUOTED(#,),) define NEW_RIGHT RIGHT_CELL(SYMBOL_TO_WRITE(OLD_TRANSITION), OLD_RIGHT, )
QUOTED(#,) define LEFT_CELL(symbol, rest, nothing) nothing ## rest
QUOTED(QUOTED(#,),) define NEW_LEFT OLD_LEFT
QUOTED(#,) undef LEFT_CELL
QUOTED(#,) define ENTERED_STACK OLD_LEFT
QUOTED(#,) endif

/* the new cell under the head is the top of the list it moved into */
QUOTED(#,) define LEFT_CELL(symbol, rest, nothing) symbol
QUOTED(#,) define RIGHT_CELL(symbol, rest, nothing) symbol
QUOTED(#,) define LEFT_EDGE O
QUOTED(#,) define RIGHT_EDGE O
QUOTED(QUOTED(#,),) define NEW_CELL ENTERED_STACK
QUOTED(#,) undef LEFT_CELL
QUOTED(#,) undef RIGHT_CELL
QUOTED(#,) undef LEFT_EDGE
QUOTED(#,) undef RIGHT_EDGE

/* --- Look up the next transition: <next state>_<new cell>, e.g. st2_O,
 *     is a #define in the Turing program. --- */
QUOTED(#,) undef tape
QUOTED(#,) include PROGRAM_FILE
QUOTED(#,) undef tape
QUOTED(#,) define stop_O HALT
QUOTED(#,) define stop_I HALT
QUOTED(#,) define LEFT_CELL(symbol, rest, nothing) _ ## symbol
QUOTED(#,) define RIGHT_CELL(symbol, rest, nothing) _ ## symbol
QUOTED(#,) define LEFT_EDGE _O
QUOTED(#,) define RIGHT_EDGE _O
QUOTED(#,) define LOOKUP_(state, suffix) state ## suffix
QUOTED(#,) define LOOKUP(state, suffix) LOOKUP_(state, suffix)
QUOTED(#,) define NEXT_TRANSITION LOOKUP(NEXT_STATE(OLD_TRANSITION), ENTERED_STACK)

QUOTED(QUOTED(#,),) define CHECK(NEXT_TRANSITION) O*I + 2*HALT
QUOTED(#,) define O 0
QUOTED(#,) define I 0
QUOTED(#,) define HALT 0
QUOTED(QUOTED(#,),) define STATUS CHECK(NEXT_TRANSITION)
QUOTED(#,) undef O
QUOTED(#,) undef I
QUOTED(#,) undef HALT
QUOTED(QUOTED(#,),) define NEW_TRANSITION NEXT_TRANSITION
QUOTED(#,) undef STATE_LABEL
QUOTED(QUOTED(#,),) define STATE_LABEL NEXT_STATE(OLD_TRANSITION).

/* --- Print the body for the next pass (see the comment at the top):
 *     the included copy skips everything up to the clean up and prints the
 *     body unquoted, then it is printed again here, still quoted. --- */
QUOTED(#,) define SKIP_BODY
QUOTED(#,) include __FILE__
QUOTED(# undef QUOTED,)
QUOTED(# define QUOTED(text, nothing) QUOTED(text, nothing),)

QUOTED(#,) endif
QUOTED(#,) endif

/* --- Forget every macro before the body is printed, so that none of the
 *     names in it get replaced.  This part runs in the included copy too. --- */
QUOTED(#,) undef __FILE__
QUOTED(#,) undef SKIP_BODY
QUOTED(#,) undef LEFT_A
QUOTED(#,) undef CELL_A
QUOTED(#,) undef RIGHT_A
QUOTED(#,) undef TRANSITION_A
QUOTED(#,) undef LEFT_B
QUOTED(#,) undef CELL_B
QUOTED(#,) undef RIGHT_B
QUOTED(#,) undef TRANSITION_B
QUOTED(#,) undef CHECK
QUOTED(#,) undef STATUS
QUOTED(#,) undef STATE_LABEL
QUOTED(#,) undef OLD_LEFT
QUOTED(#,) undef OLD_CELL
QUOTED(#,) undef OLD_RIGHT
QUOTED(#,) undef OLD_TRANSITION
QUOTED(#,) undef NEW_LEFT
QUOTED(#,) undef NEW_CELL
QUOTED(#,) undef NEW_RIGHT
QUOTED(#,) undef NEW_TRANSITION
QUOTED(#,) undef MACHINE_STOPPED
QUOTED(#,) undef BAD_STATE
QUOTED(#,) undef MOVING_RIGHT
QUOTED(#,) undef ENTERED_STACK
QUOTED(#,) undef FIRST_
QUOTED(#,) undef SECOND_
QUOTED(#,) undef THIRD_
QUOTED(#,) undef SYMBOL_TO_WRITE
QUOTED(#,) undef DIRECTION
QUOTED(#,) undef NEXT_STATE
QUOTED(#,) undef LEFT_CELL
QUOTED(#,) undef RIGHT_CELL
QUOTED(#,) undef LEFT_EDGE
QUOTED(#,) undef RIGHT_EDGE
QUOTED(#,) undef LOOKUP_
QUOTED(#,) undef LOOKUP
QUOTED(#,) undef NEXT_TRANSITION
QUOTED(#,) undef stop_O
QUOTED(#,) undef stop_I
QUOTED(#,) undef tape

#endif
