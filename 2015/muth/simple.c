/*
 * A Turing machine that runs entirely inside the C preprocessor.
 *
 * The compiled program only prints one string.  All of the real work happens
 * while the file is being compiled: the machine and the tape come from headers
 * passed with -include, and the macros below step the machine until it halts.
 *
 * Data representation
 * -------------------
 *   tape        (left, current, right)
 *   left        (left_rest, symbol)   nested to the left,  ending in (,)
 *   right       (symbol, right_rest)  nested to the right, ending in (,)
 *   transition  (write, move, next)   defined as <state>_<symbol>
 *
 * Interface with the machine and tape headers
 * -------------------------------------------
 *   tape        the initial tape
 *   sym_<sym>   marks <sym> as a real tape symbol
 *   <state>_<sym>  a transition, whose move is L, R or empty
 *   break       used in place of "move, next" to halt
 *   W(text)     turns text into a string, machine_ioccc.h writes it to the tape
 *
 * Names that cannot be changed
 * ----------------------------
 * When the machine runs out of steps, or has no transition for the current
 * state and symbol, the unfinished expansion becomes part of the printed
 * string.  That makes these otherwise cryptic macro names, and the placement
 * of some spaces, visible output: B, C, I, Q, QL, QR, Y, LY and RY.
 */

/* Token pasting; CONCAT expands its arguments first, PASTE does not */
#define PASTE(left, right) left ## right
#define CONCAT(left, right) PASTE(left, right)

#define STRINGIFY(text) #text
#define W(text) STRINGIFY(text)

#define NOTHING

/* Scan the text one more time, which runs calls that are only complete now */
#define SCAN_AGAIN(text) text

/* Pick a field out of a tape */
#define TAPE_LEFT(left, current, right) left
#define TAPE_CURRENT(left, current, right) current
#define TAPE_RIGHT(left, current, right) right

/* Pick a field out of a transition */
#define WRITTEN_SYMBOL(write, move, next) write
#define B(write, move, next) move
#define C(write, move, next) next

/* Halting replaces "move, next" with a move called HALT and an empty state */
#define break HALT ,

/*
 * sym_<symbol>(when_symbol, when_end) tells a real cell from the (,) end of the
 * known tape.  Pasting an empty symbol gives sym_, which selects when_end.  All
 * declared symbols, and the blank symbol _, select when_symbol.  The symbol __
 * selects nothing at all.
 */
#define sym_(when_symbol, when_end) when_end
#define sym__(when_symbol, when_end) when_symbol
#define sym___(when_symbol, when_end)

/* Take a cell off the left side; the end of the tape grows new blank cells */
#define LEFT_REST(rest, symbol) PASTE(sym_, symbol)(rest, (,))
#define LEFT_SYMBOL(rest, symbol) PASTE(sym_, symbol)(symbol, _ )

/* Take a cell off the right side; the end of the tape grows new blank cells */
#define RIGHT_SYMBOL(symbol, rest) PASTE(sym_, symbol)(symbol, _ )
#define RIGHT_REST(symbol, rest) PASTE(sym_, symbol)(rest, (,))

/* Rebuild the tape after writing, named by the move of the transition */
#define MOVE_TAPE_(left, written, right) left, written, right
#define MOVE_TAPE_HALT(left, written, right) left, written, right
#define MOVE_TAPE_L(left, written, right) \
    LEFT_REST left, LEFT_SYMBOL left, (written, right)
#define MOVE_TAPE_R(left, written, right) \
    (left, written), RIGHT_SYMBOL right, RIGHT_REST right

#define MOVE_TAPE(move, left, written, right) \
    CONCAT(MOVE_TAPE_, move)(left, written, right)

/* Find the transition for the state and the symbol under the head */
#define TRANSITION(state, tape) \
    CONCAT(PASTE(state, _), TAPE_CURRENT tape)

/*
 * B is followed by the name TRANSITION rather than by parentheses, so it only
 * becomes a call to B after TRANSITION has expanded and the text is scanned
 * again.  When no transition exists, this leaves "B <state>_<symbol>" behind.
 */
#define MOVEMENT(state, tape) SCAN_AGAIN(B TRANSITION(state, tape))

/* Write the new symbol and move the head, giving the next tape */
#define NEXT_TAPE(state, tape) \
    (MOVE_TAPE(MOVEMENT(state, tape), \
               TAPE_LEFT tape, \
               WRITTEN_SYMBOL TRANSITION(state, tape), \
               TAPE_RIGHT tape))

/*
 * Macros may not call themselves, so STEP cannot simply call STEP again.
 * Instead it leaves behind the text "I (O, (C, C)) (next_state, next_tape)".
 * NOTHING sits between I and its arguments, so the preprocessor does not see a
 * call to I until the text is scanned again, and by then STEP has finished.
 * I then turns into STEP, ready to take the following (state, tape) as its
 * arguments.  The first two arguments of I are ignored, and spell IOCCC.
 */
#define I(ignored_O, ignored_CC) STEP

#define NEXT_STEP(state, tape) \
    I NOTHING (O, (C, C)) (C TRANSITION(state, tape),NEXT_TAPE(state, tape))

/* What to do after this step, named by the move of the transition */
#define Q(state, tape) NEXT_STEP(state, tape)
#define QL(state, tape) NEXT_STEP(state, tape)
#define QR(state, tape) NEXT_STEP(state, tape)
#define QHALT(state, tape) NEXT_TAPE(state, tape)

/* The verbose output of a step describes the transition, or the halt */
#define Y(transition) transition
#define LY(transition) transition
#define RY(transition) transition
#define HALTY(transition) (WRITTEN_SYMBOL transition, halting)

#define DESCRIBE_TRANSITION(transition) CONCAT(B transition, Y) (transition )

/* Verbosity levels, chosen by V, with the default for an empty or missing V */
#define TRACE_(state, tape)
#define TRACE_V(state, tape) TRACE_(state, tape)
#define TRACE_0(state, tape) TRACE_(state, tape)
#define TRACE_1(state, tape) TRACE_0(state, tape)~
#define TRACE_2(state, tape) \
    TRACE_1(state, tape)tape\t<state[TAPE_CURRENT tape]>\t-> \
    DESCRIBE_TRANSITION(TRANSITION(state, tape))\n

/* Run a single step of the machine; the trace is printed directly before it */
#define STEP(state, tape) \
    CONCAT(TRACE_, V)(state, tape)CONCAT(Q, MOVEMENT(state, tape)) (state, tape )

/*
 * Every time a macro argument is scanned, one pending step gets to run.  These
 * macros scan their argument repeatedly, each level multiplying the number of
 * scans of the level below by about eight.  X picks the level and with that the
 * maximum number of machine steps, with level 3 as the default.
 */
#define EVAL_0(text) text
#define EVAL_1(text) EVAL_0(EVAL_0(EVAL_0(EVAL_0(EVAL_0(EVAL_0(EVAL_0(EVAL_0(text))))))))
#define EVAL_2(text) EVAL_1(EVAL_1(EVAL_1(EVAL_1(EVAL_1(EVAL_1(EVAL_1(EVAL_1(text))))))))
#define EVAL_3(text) EVAL_2(EVAL_2(EVAL_2(EVAL_2(EVAL_2(EVAL_2(EVAL_2(EVAL_2(text))))))))
#define EVAL_4(text) EVAL_3(EVAL_3(EVAL_3(EVAL_3(EVAL_3(EVAL_3(EVAL_3(EVAL_3(text))))))))
#define EVAL_5(text) EVAL_4(EVAL_4(EVAL_4(EVAL_4(EVAL_4(EVAL_4(EVAL_4(EVAL_4(text))))))))
#define EVAL_6(text) EVAL_5(EVAL_5(EVAL_5(EVAL_5(EVAL_5(EVAL_5(EVAL_5(EVAL_5(text))))))))
#define EVAL_7(text) EVAL_6(EVAL_6(EVAL_6(EVAL_6(EVAL_6(EVAL_6(EVAL_6(EVAL_6(text))))))))
#define EVAL_8(text) EVAL_7(EVAL_7(EVAL_7(EVAL_7(EVAL_7(EVAL_7(EVAL_7(EVAL_7(text))))))))
#define EVAL_9(text) EVAL_8(EVAL_8(EVAL_8(EVAL_8(EVAL_8(EVAL_8(EVAL_8(EVAL_8(text))))))))
#define EVAL_(text) EVAL_3(text)
#define EVAL_X(text) EVAL_3(text)

/*
 * Run the machine and turn the final tape into a string.  This has to scan with
 * Y instead of SCAN_AGAIN because a macro is not expanded while it is scanning
 * its own result, and the steps of that last scan still need SCAN_AGAIN.
 */
#define RUN_MACHINE(state, tape) \
    W(Y(CONCAT(EVAL_, X)(STEP(state, tape))))

int puts (char*);

int main ()
{
    puts(RUN_MACHINE(A, tape));
}
