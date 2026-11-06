#include <stdlib.h>
#include <stdio.h>

/*
 * Unlambda 2.0 interpreter.
 *
 * Every Unlambda value is a "node": an array of NODE_SLOTS cells.
 *
 *   node[FUNC]  the C function that runs when the node is applied
 *               (for an unevaluated application tree it holds eval_tree
 *               as a tag instead)
 *   node[ARG1]  first stored argument  (a node, or a character)
 *   node[ARG2]  second stored argument (a node, or a file position)
 *   node[LINK]  non-NULL while the C stack is being unwound to capture or
 *               throw a continuation; links to the next inner stack frame
 *   node[REFS]  reference count
 *
 * The source file is parsed lazily while it is being executed.
 */

typedef union Cell Cell;
union Cell {
	Cell *(*build)(Cell *tmpl);            /* parser / constructor */
	Cell *(*apply)(Cell *self, Cell *arg); /* function body */
	Cell *node;
	long long number;                      /* char, file position, count */
};

enum { FUNC, ARG1, ARG2, LINK, REFS, NODE_SLOTS };

/* classes of characters that are skipped between tokens */
enum { SKIP_BLANK = 1, SKIP_COMMENT = 2 };

void die(char *message, int status)
{
	fputs(message, stderr);
	exit(status);
}

void syntax_error(void)
{
	die("syntax error\n", 1);
}

void fail(void)
{
	die("fail\n", -1);
}

Cell *ref(Cell *f)
{
	f[REFS].number++;
	return f;
}

/* parsers */
Cell *new_node(Cell *tmpl);
Cell *parse_char_builtin(Cell *tmpl);
Cell *parse_newline_builtin(Cell *tmpl);
Cell *parse_apply(Cell *tmpl);
Cell *parse_apply_lazily(Cell *tmpl);
Cell *parse_apply_discard(Cell *tmpl);

/* evaluation of application nodes */
Cell *apply_operator(Cell *op);
Cell *resume_apply_operator(Cell *frame, Cell *value);
Cell *apply_to_operand(Cell *op, Cell *arg);
Cell *resume_apply_to_operand(Cell *frame, Cell *value);
Cell *eval_tree(Cell *tree);
Cell *eval_tree_operand(Cell *tree, Cell *op);
Cell *resume_eval_tree_operand(Cell *frame, Cell *value);
Cell *eval_tree_apply(Cell *op, Cell *arg);
Cell *resume_eval_tree_apply(Cell *frame, Cell *value);

/* builtins */
Cell *i_builtin(Cell *self, Cell *arg);
Cell *v_builtin(Cell *self, Cell *arg);
Cell *k_builtin(Cell *self, Cell *arg);
Cell *k_with_x(Cell *self, Cell *arg);
Cell *s_builtin(Cell *self, Cell *arg);
Cell *s_with_x(Cell *self, Cell *arg);
Cell *s_with_xy(Cell *self, Cell *arg);
Cell *s_eval_yz(Cell *self, Cell *xz, Cell *z);
Cell *resume_s_eval_yz(Cell *frame, Cell *value);
Cell *s_apply_results(Cell *xz, Cell *yz);
Cell *resume_s_apply_results(Cell *frame, Cell *value);
Cell *d_builtin(Cell *self, Cell *arg);
Cell *promise(Cell *self, Cell *arg);
Cell *promise_apply(Cell *func, Cell *arg);
Cell *resume_promise_apply(Cell *frame, Cell *value);
Cell *print_builtin(Cell *self, Cell *arg);
Cell *read_builtin(Cell *self, Cell *arg);
Cell *compare_builtin(Cell *self, Cell *arg);
Cell *reprint_builtin(Cell *self, Cell *arg);
Cell *callcc_builtin(Cell *self, Cell *arg);
Cell *callcc_point(Cell *self, Cell *arg);
Cell *continuation(Cell *self, Cell *arg);
Cell *jump_signal(Cell *self, Cell *arg);
Cell *exit_builtin(Cell *self, Cell *arg);
Cell *exit_signal(Cell *self, Cell *arg);

void unref(Cell *f)
{
	if (f && !--f[REFS].number) {
		/* print and compare keep a character, not a node, in ARG1 */
		if (f[FUNC].apply != print_builtin && f[FUNC].apply != compare_builtin)
			unref(f[ARG1].node);
		/* a continuation keeps a file position in ARG2 */
		if (f[FUNC].apply != continuation)
			unref(f[ARG2].node);
		/* signal nodes link to themselves */
		if (f[FUNC].apply != callcc_point && f[FUNC].apply != jump_signal &&
		    f[FUNC].apply != exit_signal)
			unref(f[LINK].node);
		free(f);
	}
}

int last_input = EOF;   /* last character read by @ */
int current;            /* look-ahead character of the source */
FILE *source;

/*
 * Translation table: for each source character, slot 0 is the parser
 * that turns it into a node and slot 1 is the template copied into the
 * new node (the builtin's function). Characters without a parser have a
 * skip class in slot 1, or 0 when they are not allowed.
 */
Cell table[256][2] = {
	['\t'] = { [1].number = SKIP_BLANK },
	['\n'] = { [1].number = SKIP_BLANK },
	['\v'] = { [1].number = SKIP_BLANK },
	['\f'] = { [1].number = SKIP_BLANK },
	['\r'] = { [1].number = SKIP_BLANK },
	[' ']  = { [1].number = SKIP_BLANK },
	['#']  = { [1].number = SKIP_COMMENT },
	['.']  = { { parse_char_builtin },    [1].apply = print_builtin },
	['?']  = { { parse_char_builtin },    [1].apply = compare_builtin },
	['@']  = { { new_node },              [1].apply = read_builtin },
	['`']  = { { parse_apply } },
	['c']  = { { new_node },              [1].apply = callcc_builtin },
	['d']  = { { new_node },              [1].apply = d_builtin },
	['e']  = { { new_node },              [1].apply = exit_builtin },
	['i']  = { { new_node },              [1].apply = i_builtin },
	['k']  = { { new_node },              [1].apply = k_builtin },
	['r']  = { { parse_newline_builtin }, [1].apply = print_builtin },
	['s']  = { { new_node },              [1].apply = s_builtin },
	['v']  = { { new_node },              [1].apply = v_builtin },
	['|']  = { { new_node },              [1].apply = reprint_builtin },
};

/* skip whitespace and # comments up to the next token */
void skip_blanks(void)
{
	for (;;) {
		if (current == EOF)
			return;
		if (table[current][0].build)
			return;
		if (!table[current][1].number)
			syntax_error();
		if (table[current][1].number == SKIP_COMMENT)
			for (current = getc(source); current != '\n' && current != EOF;
			     current = getc(source))
				;
		current = getc(source);
	}
}

void next_token(void)
{
	current = getc(source);
	skip_blanks();
}

Cell *parse_expression(void)
{
	if (current == EOF)
		syntax_error();
	Cell *entry = table[current];
	if (entry[0].build != parse_char_builtin)
		next_token();
	return entry[0].build(&entry[1]);
}

Cell *new_node(Cell *tmpl)
{
	Cell *node = malloc(sizeof(Cell[NODE_SLOTS]));
	if (!node)
		die("memory error\n", -2);
	node[FUNC] = *tmpl;
	for (int i = 1; i < NODE_SLOTS; i++)
		node[i] = (Cell){0};
	return ref(node);
}

Cell *new_function(Cell *(*body)(Cell *, Cell *))
{
	return new_node(&(Cell){ .apply = body });
}

int main(int argc, const char **argv)
{
	if (argc != 2)
		fail();
	source = fopen(argv[1], "rb");
	if (!source)
		fail();
	next_token();

	/* run every program in the file */
	while (current != EOF) {
		Cell *result = parse_expression();

		/* the C stack was unwound to the top: handle the signal */
		while (result[LINK].node) {
			if (result[FUNC].apply == jump_signal) {
				/* rebuild the saved stack, then deliver the value */
				Cell *saved = result[ARG1].node;
				result = saved[FUNC].apply(ref(saved), result);
			} else if (result[FUNC].apply == exit_signal) {
				table['`'][0].build = parse_apply;
				Cell *value = ref(result[ARG1].node);
				unref(result);
				result = value;
			} else {
				/* c was called: wrap the unwound stack into a continuation
				 * and rebuild the stack passing the continuation down */
				Cell *cont = new_function(continuation);
				cont[ARG1].node = result;
				cont[ARG2].number = ftell(source) + (current == EOF);
				result = result[FUNC].apply(ref(result), cont);
			}
		}
		unref(result);
	}
	return 0;
}

/* .x and ?x : remember the character after the prefix */
Cell *parse_char_builtin(Cell *tmpl)
{
	Cell *node = new_node(tmpl);
	current = getc(source);
	if (current == EOF)
		syntax_error();
	node[ARG1].number = current;
	next_token();
	return node;
}

/* r is .x with a newline */
Cell *parse_newline_builtin(Cell *tmpl)
{
	Cell *node = new_node(tmpl);
	node[ARG1].number = '\n';
	return node;
}

/* thrown value arrives at the place where c was called */
Cell *jump_signal(Cell *self, Cell *arg)
{
	Cell *value = ref(self[ARG2].node);
	unref(arg);
	unref(self);
	return value;
}

/* d x -> promise of x (x was not evaluated) */
Cell *d_builtin(Cell *self, Cell *arg)
{
	Cell *node = new_function(promise);
	node[ARG1].node = arg;
	unref(self);
	return node;
}

/* evaluate tree: second step, apply operator to the evaluated operand */
Cell *eval_tree_apply(Cell *op, Cell *arg)
{
	if (arg[LINK].node) {
		if (arg[FUNC].apply == jump_signal || arg[FUNC].apply == exit_signal) {
			unref(op);
			return arg;
		}
		Cell *frame = new_function(resume_eval_tree_apply);
		frame[ARG1].node = op;
		frame[LINK].node = arg;
		return frame;
	}
	return op[FUNC].apply(op, arg);
}

Cell *resume_eval_tree_apply(Cell *frame, Cell *value)
{
	Cell *op = ref(frame[ARG1].node);
	Cell *inner = ref(frame[LINK].node);
	unref(frame);
	return eval_tree_apply(op, inner[FUNC].apply(inner, value));
}

/* evaluate tree: first step, operator evaluated, now evaluate operand */
Cell *eval_tree_operand(Cell *tree, Cell *op)
{
	if (op[LINK].node) {
		if (op[FUNC].apply == jump_signal || op[FUNC].apply == exit_signal)
			return op;
		Cell *frame = new_function(resume_eval_tree_operand);
		frame[ARG1].node = tree;
		frame[LINK].node = op;
		return frame;
	}
	if (op[FUNC].apply == d_builtin) {
		/* `d F does not evaluate F */
		Cell *node = new_function(promise);
		node[ARG1].node = ref(tree[ARG2].node);
		unref(op);
		unref(tree);
		return node;
	}
	Cell *arg = eval_tree(tree[ARG2].node);
	unref(tree);
	return eval_tree_apply(op, arg);
}

Cell *resume_eval_tree_operand(Cell *frame, Cell *value)
{
	Cell *tree = ref(frame[ARG1].node);
	Cell *inner = ref(frame[LINK].node);
	unref(frame);
	return eval_tree_operand(tree, inner[FUNC].apply(inner, value));
}

/* evaluate a stored (unevaluated) application tree */
Cell *eval_tree(Cell *tree)
{
	ref(tree);
	if (tree[FUNC].build != eval_tree)
		return tree;
	return eval_tree_operand(tree, eval_tree(tree[ARG1].node));
}

Cell *promise_apply(Cell *func, Cell *arg)
{
	if (func[LINK].node) {
		if (func[FUNC].apply == jump_signal || func[FUNC].apply == exit_signal) {
			unref(arg);
			return func;
		}
		Cell *frame = new_function(resume_promise_apply);
		frame[ARG1].node = arg;
		frame[LINK].node = func;
		return frame;
	}
	return func[FUNC].apply(func, arg);
}

Cell *resume_promise_apply(Cell *frame, Cell *value)
{
	Cell *inner = ref(frame[LINK].node);
	Cell *arg = ref(frame[ARG1].node);
	unref(frame);
	return promise_apply(inner[FUNC].apply(inner, value), arg);
}

/* forcing a promise: evaluate the delayed tree then apply it */
Cell *promise(Cell *self, Cell *arg)
{
	Cell *func = eval_tree(self[ARG1].node);
	unref(self);
	return promise_apply(func, arg);
}

/* ` inside a d operand: build a tree without evaluating it */
Cell *parse_apply_lazily(Cell *tmpl)
{
	(void)tmpl;
	Cell *tree = new_node(&(Cell){ .build = eval_tree });
	tree[ARG1].node = parse_expression();
	tree[ARG2].node = parse_expression();
	return tree;
}

/* ` while exiting: just skip over the source */
Cell *parse_apply_discard(Cell *tmpl)
{
	(void)tmpl;
	unref(parse_expression());
	return parse_expression();
}

Cell *apply_to_operand(Cell *op, Cell *arg)
{
	if (arg[LINK].node) {
		if (arg[FUNC].apply == jump_signal || arg[FUNC].apply == exit_signal) {
			unref(op);
			return arg;
		}
		Cell *frame = new_function(resume_apply_to_operand);
		frame[ARG1].node = op;
		frame[LINK].node = arg;
		return frame;
	}
	return op[FUNC].apply(op, arg);
}

Cell *resume_apply_to_operand(Cell *frame, Cell *value)
{
	Cell *op = ref(frame[ARG1].node);
	Cell *inner = ref(frame[LINK].node);
	unref(frame);
	return apply_to_operand(op, inner[FUNC].apply(inner, value));
}

/* ` from the source: operator evaluated, now parse and evaluate operand */
Cell *apply_operator(Cell *op)
{
	if (op[LINK].node) {
		if (op[FUNC].apply == jump_signal)
			return op;
		if (op[FUNC].apply == exit_signal) {
			unref(parse_expression());
			return op;
		}
		Cell *frame = new_function(resume_apply_operator);
		frame[LINK].node = op;
		return frame;
	}
	if (op[FUNC].apply != d_builtin)
		return apply_to_operand(op, parse_expression());

	/* `d F : read F as a tree without evaluating it */
	unref(op);
	table['`'][0].build = parse_apply_lazily;
	Cell *node = new_function(promise);
	node[ARG1].node = parse_expression();
	table['`'][0].build = parse_apply;
	return node;
}

Cell *resume_apply_operator(Cell *frame, Cell *value)
{
	Cell *inner = ref(frame[LINK].node);
	unref(frame);
	return apply_operator(inner[FUNC].apply(inner, value));
}

Cell *parse_apply(Cell *tmpl)
{
	(void)tmpl;
	return apply_operator(parse_expression());
}

/* `kx y -> x */
Cell *k_with_x(Cell *self, Cell *arg)
{
	Cell *x = ref(self[ARG1].node);
	unref(arg);
	unref(self);
	return x;
}

Cell *k_builtin(Cell *self, Cell *arg)
{
	Cell *node = new_function(k_with_x);
	node[ARG1].node = arg;
	unref(self);
	return node;
}

/* ``sxyz -> ``xz`yz : last step, apply (xz) to (yz) */
Cell *s_apply_results(Cell *xz, Cell *yz)
{
	if (yz[LINK].node) {
		if (yz[FUNC].apply == jump_signal || yz[FUNC].apply == exit_signal) {
			unref(xz);
			return yz;
		}
		Cell *frame = new_function(resume_s_apply_results);
		frame[ARG1].node = xz;
		frame[LINK].node = yz;
		return frame;
	}
	return xz[FUNC].apply(xz, yz);
}

Cell *resume_s_apply_results(Cell *frame, Cell *value)
{
	Cell *xz = ref(frame[ARG1].node);
	Cell *inner = ref(frame[LINK].node);
	unref(frame);
	return s_apply_results(xz, inner[FUNC].apply(inner, value));
}

/* ``sxyz : xz is known, now compute yz */
Cell *s_eval_yz(Cell *self, Cell *xz, Cell *z)
{
	if (xz[LINK].node) {
		if (xz[FUNC].apply == jump_signal || xz[FUNC].apply == exit_signal) {
			unref(z);
			unref(self);
			return xz;
		}
		Cell *frame = new_function(resume_s_eval_yz);
		frame[ARG1].node = self;
		frame[ARG2].node = z;
		frame[LINK].node = xz;
		return frame;
	}
	if (xz[FUNC].apply == d_builtin) {
		/* `d`yz : delay the evaluation of `yz */
		Cell *node = new_function(promise);
		node[ARG1].node = new_node(&(Cell){ .build = eval_tree });
		node[ARG1].node[ARG1].node = ref(self[ARG2].node);
		node[ARG1].node[ARG2].node = z;
		unref(self);
		return node;
	}
	Cell *y = ref(self[ARG2].node);
	unref(self);
	return s_apply_results(xz, y[FUNC].apply(y, z));
}

Cell *resume_s_eval_yz(Cell *frame, Cell *value)
{
	Cell *inner = ref(frame[LINK].node);
	Cell *z = ref(frame[ARG2].node);
	Cell *self = ref(frame[ARG1].node);
	unref(frame);
	return s_eval_yz(self, inner[FUNC].apply(inner, value), z);
}

Cell *s_with_xy(Cell *self, Cell *z)
{
	Cell *x = ref(self[ARG1].node);
	ref(z);
	return s_eval_yz(self, x[FUNC].apply(x, z), z);
}

Cell *s_with_x(Cell *self, Cell *y)
{
	Cell *node = new_function(s_with_xy);
	node[ARG1].node = ref(self[ARG1].node);
	node[ARG2].node = y;
	unref(self);
	return node;
}

Cell *s_builtin(Cell *self, Cell *x)
{
	Cell *node = new_function(s_with_x);
	node[ARG1].node = x;
	unref(self);
	return node;
}

Cell *i_builtin(Cell *self, Cell *arg)
{
	unref(self);
	return arg;
}

Cell *v_builtin(Cell *self, Cell *arg)
{
	unref(arg);
	return self;
}

/* .x : print x and act as identity */
Cell *print_builtin(Cell *self, Cell *arg)
{
	putc(self[ARG1].number, stdout);
	unref(self);
	return arg;
}

/* @ : read a character, `x i on success, `x v on EOF */
Cell *read_builtin(Cell *self, Cell *arg)
{
	last_input = getc(stdin);
	Cell *result = new_function(last_input != EOF ? i_builtin : v_builtin);
	unref(self);
	return arg[FUNC].apply(arg, result);
}

/* ?x : `y i if the last character read is x, else `y v */
Cell *compare_builtin(Cell *self, Cell *arg)
{
	Cell *result = new_function(last_input == self[ARG1].number ? i_builtin : v_builtin);
	unref(self);
	return arg[FUNC].apply(arg, result);
}

/* | : `y .x with the last character read, or `y v on EOF */
Cell *reprint_builtin(Cell *self, Cell *arg)
{
	Cell *result = new_function(last_input != EOF ? print_builtin : v_builtin);
	if (last_input != EOF)
		result[ARG1].number = last_input;
	unref(self);
	return arg[FUNC].apply(arg, result);
}

/*
 * Marker left where c was applied. While the stack is rebuilt for the
 * first time it calls the argument of c with the continuation; when a
 * thrown value arrives it returns that value.
 */
Cell *callcc_point(Cell *self, Cell *arg)
{
	if (arg[LINK].node == arg)
		return arg[FUNC].apply(arg, self);
	Cell *func = ref(self[ARG1].node);
	unref(self);
	return func[FUNC].apply(func, arg);
}

/* c x : start unwinding the C stack to capture the continuation */
Cell *callcc_builtin(Cell *self, Cell *arg)
{
	Cell *marker = new_function(callcc_point);
	marker[ARG1].node = arg;
	marker[LINK].node = marker;
	unref(self);
	return marker;
}

/* applying a continuation: rewind the source and throw the value */
Cell *continuation(Cell *self, Cell *value)
{
	Cell *signal = new_function(jump_signal);
	signal[ARG1].node = ref(self[ARG1].node);
	signal[ARG2].node = value;
	signal[LINK].node = signal;
	if (fseek(source, self[ARG2].number - 1, SEEK_SET))
		fail();
	next_token();
	unref(self);
	return signal;
}

Cell *exit_signal(Cell *self, Cell *arg)
{
	(void)self;
	return arg;
}

/* e x : unwind everything and finish the current program with x */
Cell *exit_builtin(Cell *self, Cell *arg)
{
	Cell *signal = new_function(exit_signal);
	signal[ARG1].node = arg;
	signal[LINK].node = signal;
	table['`'][0].build = parse_apply_discard;
	unref(self);
	return signal;
}
