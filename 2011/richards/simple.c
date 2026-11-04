#ifdef AH
#include <alloca.h>
#endif
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifndef NM
#include <sys/mman.h>
#endif

/*
 * A tiny dc (reverse Polish calculator) with a just-in-time compiler.
 *
 * The JIT contains no assembler.  Instead, code_template() below is an
 * ordinary C function holding one snippet of C for every dc operation.
 * Between the snippets it calls d->hook(id).  When the program starts,
 * hook points at record(), which looks up its own return address on the
 * stack to learn where each snippet's machine code begins and ends.
 * Compiling a line of dc then just means copying those machine code
 * fragments one after another into an executable buffer.  When the
 * compiled code runs, hook points at a function that does nothing.
 */

#define TABLE_SIZE 589824 /* 9 << 16: size of every big table / buffer */
#define NUM_REGISTERS 256
#define MAX_SMALL_NUMBER 256 /* largest number with its own push fragment */

/* fragment ids: operator fragments use the operator's character */
#define FRAG_PROLOGUE 1 /* start of code_template up to the first hook */
#define FRAG_EPILOGUE 2 /* code after the last hook up to the return */
#define FRAG_COND_EXEC '|'
#define FRAG_PUSH(v) (NUM_REGISTERS + (v)) /* fragment that pushes v */

/* slots of the library table, for operations that call other functions */
#define LIB_NOTHING 0
#define LIB_DIVIDE 1
#define LIB_PRINT 2
#define LIB_EXECUTE 3
#define LIB_COND_EXECUTE 4

typedef struct machine *machine_t;
typedef void (*code_fn)(volatile machine_t);

struct machine {
  int *sp;           /* top of the value stack */
  int **registers;   /* top of each register's own stack */
  int *values;       /* values[v] == v; read by the push fragments */
  int num_strings;   /* number of macro strings seen so far */
  char **strings;    /* text of each [macro] */
  code_fn *lib;      /* helper functions called by the fragments */
  code_fn *macros;   /* compiled code of each macro (compiled lazily) */
  void (*hook)(int); /* called between fragments */
};

int values[MAX_SMALL_NUMBER + 1];
machine_t machine;
int stack[TABLE_SIZE];
char *template_pos; /* how far code_template has been cut into fragments */
void *fragment_start[TABLE_SIZE];
int fragment_size[TABLE_SIZE];
char *code_end; /* end of the generated code */

/* append the machine code of one fragment to the generated code */
void emit(int id)
{
  code_end = (char *)memmove(code_end, fragment_start[id], fragment_size[id]) +
             fragment_size[id];
}

/* generate code that pushes a number (built from base 256 digits) */
void emit_push(int value)
{
  if (value > MAX_SMALL_NUMBER) {
    emit_push(value / 256);
    emit_push(256);
    emit('*');
    emit_push(value % 256);
    emit('+');
  } else {
    emit(FRAG_PUSH(value));
  }
}

/* translate one line of dc into machine code and return it as a function */
code_fn compile(char *p, volatile machine_t d)
{
  char c;
  int depth, negate;
  code_fn code = (code_fn)code_end;

  emit(FRAG_PROLOGUE);
  while ((c = *p)) {
    switch (c) {
    case '#': /* comment up to the end of the line */
      while (*++p != '\n')
        ;
      break;

    case '[': /* macro string: remember the text, push its number */
      d->strings[d->num_strings] = ++p;
      for (depth = 1; depth; p++) {
        if (*p == '[')
          depth++;
        else if (*p == ']')
          depth--;
      }
      p[-1] = '\0';
      emit_push(d->num_strings++);
      break;

    case '!': /* !<r  !>r  !=r */
    case '<': /* <r  >r  =r: compare, then maybe run register r */
    case '>':
    case '=':
      negate = 0;
      if (c == '!') {
        negate = 1;
        c = *++p;
      }
      /*
       * dc compares the top against the value below it, but the template's
       * fragments compare the value below against the top, so turn
       * '<' into '>' and '>' into '<' (they differ only in bit 1).
       */
      if (!(c & 1))
        c ^= 2;
      emit(c);
      if (negate)
        emit('!');
      emit_push(*++p); /* register name */
      emit(FRAG_COND_EXEC);
      p++;
      break;

    case 's': /* register commands: next character names the register */
    case 'S':
    case 'l':
    case 'L':
      emit_push(*++p);
      emit(c);
      p++;
      break;

    case '_': /* negative number, or negate when no digits follow */
      p++;
      /* fall through */
    default:
      if (isdigit(*p)) {
        emit_push(atoi(p));
        while (isdigit(*p))
          p++;
      } else {
        p++;
      }
      /* the operator's fragment; characters without one emit nothing */
      emit(c);
    }
  }
  emit(FRAG_EPILOGUE);
  return code;
}

/* one fragment for every value 0..256 that pushes that value */
#define PUSH(v)                                                                \
  *++d->sp = d->values[v];                                                     \
  d->hook(FRAG_PUSH(v));
#define PUSH4(v) PUSH(v) PUSH(v + 1) PUSH(v + 2) PUSH(v + 3)
#define PUSH16(v) PUSH4(v) PUSH4(v + 4) PUSH4(v + 8) PUSH4(v + 12)
#define PUSH64(v) PUSH16(v) PUSH16(v + 16) PUSH16(v + 32) PUSH16(v + 48)
#define PUSH256(v) PUSH64(v) PUSH64(v + 64) PUSH64(v + 128) PUSH64(v + 192)

/*
 * The machine code of every operation, separated by calls to d->hook.
 * Everything is reached through the volatile pointer d (kept on the stack)
 * so that each fragment works no matter which fragments surround it, and
 * anything needing a call (division, printf, ...) goes through d->lib so
 * that the copied code contains no position dependent calls.
 */
void code_template(volatile machine_t d)
{
  d->hook(FRAG_PROLOGUE);

  d->sp--;
  *d->sp = *d->sp + d->sp[1];
  d->hook('+');

  d->sp--;
  *d->sp = *d->sp - d->sp[1];
  d->hook('-');

  d->sp--;
  *d->sp = *d->sp * d->sp[1];
  d->hook('*');

  *d->sp = -*d->sp; /* negate */
  d->hook('_');

  d->sp--;
  *d->sp = *d->sp == d->sp[1];
  d->hook('=');

  d->sp--;
  *d->sp = *d->sp < d->sp[1];
  d->hook('<');

  d->sp--;
  *d->sp = *d->sp > d->sp[1];
  d->hook('>');

  *d->sp = !*d->sp;
  d->hook('!');

  /* "swap": only two of the three steps of an xor swap */
  d->sp[-1] ^= *d->sp;
  *d->sp ^= d->sp[-1];
  d->hook('r');

  d->sp[1] = *d->sp; /* duplicate */
  d->sp++;
  d->hook('d');

  /* sr: store into register r (the value stays on the stack) */
  *d->registers[*d->sp] = d->sp[-1];
  d->sp--;
  d->hook('s');

  /* Sr: push onto register r's stack (the value stays on the stack) */
  *++d->registers[*d->sp] = d->sp[-1];
  d->sp--;
  d->hook('S');

  /* lr: load from register r */
  *d->sp = *d->registers[*d->sp];
  d->hook('l');

  /* Lr: pop from register r's stack */
  *d->sp = *d->registers[*d->sp]--;
  d->hook('L');

  d->lib[LIB_DIVIDE](d);
  d->hook('/');

  d->lib[LIB_PRINT](d);
  d->hook('p');

  d->lib[LIB_EXECUTE](d);
  d->hook('x');

  d->lib[LIB_COND_EXECUTE](d);
  d->hook(FRAG_COND_EXEC);

  PUSH256(0)
  PUSH(256)

  /* a final call so that the last hook is not turned into a jump */
  d->lib[LIB_NOTHING](d);
}

/* must directly follow code_template: marks where its code ends */
void template_end()
{
}

/*
 * Called (through record) by d->hook while code_template runs once at
 * start up.  The return address into code_template is somewhere on the
 * stack just above this function's local variables: everything between
 * the previous return address and this one is fragment id.
 */
void record_fragment(int id)
{
  void *anchor = 0, **slot = &anchor, **limit = slot + 32;

  fragment_start[id] = template_pos;
  for (; slot < limit &&
         (*slot < (void *)template_pos || *slot > (void *)template_end ||
          *slot == &id);
       slot++)
    ;
  if (slot == limit) {
    /*
     * Not found (the return address is still in a register): use up some
     * stack, force the registers onto it with setjmp and look again.
     */
    jmp_buf *saved = alloca(sizeof(*saved));
    setjmp(*saved);
    record_fragment(id);
  } else {
    fragment_size[id] = *slot - (void *)template_pos;
    template_pos = *slot;
  }
}

/* volatile so the compiler cannot inline record_fragment into record */
void (*volatile record_fragment_ptr)(int) = record_fragment;

void record(int id)
{
  /* give the template a fresh stack for every operation it tries out */
  *(machine->sp = stack + 9) = 0;
  record_fragment_ptr(id);
}

void lib_divide(volatile machine_t d)
{
  d->sp--;
  *d->sp /= d->sp[1];
}

void lib_print(volatile machine_t d)
{
  printf("%d\n", *d->sp);
}

/* x: run the macro whose number is on top of the stack */
void lib_execute(volatile machine_t d)
{
  int n = *d->sp--;

  if (!d->macros[n])
    d->macros[n] = compile(d->strings[n], d);
  d->macros[n](d);
}

/* run the macro in a register if the comparison result below it is true */
void lib_cond_execute(volatile machine_t d)
{
  d->sp -= 2;
  if (d->sp[1]) {
    d->sp++;
    *d->sp = *d->registers[d->sp[1]];
    lib_execute(d);
  }
}

/* library used while recording: nothing may actually be called */
code_fn record_lib[] = {template_end, template_end, template_end, template_end,
                        template_end};
code_fn run_lib[] = {template_end, lib_divide, lib_print, lib_execute,
                     lib_cond_execute};

int main()
{
  int i;
  char *line;
  machine_t d = machine = calloc(sizeof(*machine), 1);

  d->sp = stack + 9;
  d->registers = calloc(NUM_REGISTERS * sizeof(char *), 1);
  for (i = 0; i < NUM_REGISTERS; i++)
    d->registers[i] = calloc(TABLE_SIZE, 1);
  d->values = values;
  d->strings = calloc(TABLE_SIZE, 1);
  d->lib = record_lib;
  d->macros = calloc(TABLE_SIZE, 1);
  d->hook = record;
  for (i = 0; i <= MAX_SMALL_NUMBER; i++)
    values[i] = i;

  /* run the template once to cut it into fragments */
  template_pos = (char *)code_template;
  code_template(d);
  fragment_start[FRAG_EPILOGUE] = template_pos;
  fragment_size[FRAG_EPILOGUE] = (char *)template_end - template_pos + 8;

  d->lib = run_lib;
  d->hook = (void (*)(int))template_end;
  code_end =
#ifdef NM
      malloc(TABLE_SIZE);
#else
      mmap(NULL, TABLE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC,
           MAP_ANON | MAP_PRIVATE, -1, 0);
#endif
  while ((line = fgets(malloc(999), 999, stdin)))
    compile(line, d)(d);
}
