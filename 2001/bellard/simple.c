#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

/*
 * OTCC - Obfuscated Tiny C Compiler (deobfuscated).
 *
 * The compiler reads C source and writes i386 machine code straight into an
 * executable memory buffer, then calls the compiled main().
 *
 * Every pointer is stored in a plain 'int' (this only works on 32-bit i386),
 * because the generated code and the symbol table both use 4-byte slots.
 */

void *dlsym(void *handle, const char *name); /* resolved from libdl */

#define BUFFER_SIZE 99999

/*
 * Keyword table. An identifier token is its offset in the symbol name
 * buffer * 8 + TOK_IDENT, so the keywords get these fixed token numbers.
 */
#define KEYWORDS " int if else while break return for define main "
#define KEYWORDS_LENGTH 48
#define TOK_IDENT 256
#define TOK_INT 256    /* offset  0 */
#define TOK_IF 288     /* offset  4 */
#define TOK_ELSE 312   /* offset  7 */
#define TOK_WHILE 352  /* offset 12 */
#define TOK_BREAK 400  /* offset 18 */
#define TOK_RETURN 448 /* offset 24 */
#define TOK_FOR 504    /* offset 31 */
#define TOK_DEFINE 536 /* offset 35 */
#define TOK_MAIN 592   /* offset 42 */

#define TOK_DUMMY 1 /* second char of a two-char operator was consumed */
#define TOK_NUM 2   /* number or character constant, value in token_value */

#define LOCAL_LIMIT 512 /* symbol values below this are %ebp offsets */
#define SYM_DEFINE 1    /* symbol value marking a #define macro */

#define MACRO_END 2 /* marks the end of a macro body */

/* precedence levels of the operators (token_level) */
#define LEVEL_UNARY 2
#define LEVEL_COMPARE 4
#define LEVEL_EQUALITY 5
#define LEVEL_LOGICAL 8 /* levels above this are && and || */
#define LEVEL_POSTFIX 11
#define LEVEL_TOP 11

/* i386 instruction encodings (emitted low byte first) */
#define OP_PUSH_EAX 0x50
#define OP_POP_ECX 0x59
#define OP_XCHG_EDX_EAX 0x92
#define OP_MOV_IMM_EAX 0xb8
#define OP_MOV_IMM_ECX 0xb9
#define OP_CALL 0xe8
#define OP_JMP 0xe9
#define OP_TEST_EAX_JCC 0x0fc085 /* test %eax,%eax ; 0f (jcc) */
#define OP_JE 0x84               /* second byte of je / jne */
#define OP_CMP_EAX_ECX 0xc139    /* cmp %eax,%ecx */
#define OP_SETCC 0x0f            /* first byte of setcc */
#define OP_SETCC_BASE 0x90       /* second byte of setcc */
#define OP_MODRM_AL 0xc0         /* setcc %al */
#define OP_STORE_ECX 0x188       /* mov %al,(%ecx) ; +1 = mov %eax,(%ecx) */
#define OP_LOAD_INT 0x8b         /* mov (%eax),%eax (plus a 0 byte) */
#define OP_LOAD_CHAR 0xbe0f      /* movsbl (%eax),%eax (plus a 0 byte) */
#define OP_SUB_ESP 0xec81        /* sub $imm32,%esp */
#define OP_ADD_ESP 0xc481        /* add $imm32,%esp */
#define OP_STORE_ARG 0x248489    /* mov %eax,imm32(%esp) */
#define OP_CALL_ARG 0x2494ff     /* call *imm32(%esp) */
#define OP_PROLOGUE 0xe58955     /* push %ebp ; mov %esp,%ebp */
#define OP_EPILOGUE 0xc3c9       /* leave ; ret */
#define OP_RM_ADD 0x83           /* base opcode for emit_variable_access */
#define ACCESS_ADD_IMM8 0        /* addl $imm8,EA  (0x83) */
#define ACCESS_STORE 6           /* mov %eax,EA    (0x89) */
#define ACCESS_LOAD 8            /* mov EA,%eax    (0x8b) */
#define ACCESS_ADDRESS 10        /* lea EA,%eax    (0x8d) */

/*
 * Operator table. 'second' is the required second character, or '@' for a
 * single character operator. 'code' is the machine code (or condition code /
 * increment) used for the operator, and 'level' its precedence level.
 */
struct operator_info {
  char first, second;
  int code;
  int level;
};

struct operator_info operators[] = {
    {'+', '+', 0x01, 11},      /* postfix ++: addl $1    */
    {'-', '-', 0xff, 11},      /* postfix --: addl $-1   */
    {'*', '@', 0xc1af0f, 1},   /* imul %ecx,%eax         */
    {'/', '@', 0xf9f79991, 1}, /* xchg; cltd; idiv %ecx  */
    {'%', '@', 0xf9f79991, 1}, /* same, then xchg %edx   */
    {'+', '@', 0xc801, 2},     /* add %ecx,%eax          */
    {'-', '@', 0xd8f7c829, 2}, /* sub %ecx,%eax; neg %eax */
    {'<', '<', 0xe0d391, 3},   /* xchg; shl %cl,%eax     */
    {'>', '>', 0xf8d391, 3},   /* xchg; sar %cl,%eax     */
    {'<', '=', 0x0e, 4},       /* setle */
    {'>', '=', 0x0d, 4},       /* setge */
    {'<', '@', 0x0c, 4},       /* setl  */
    {'>', '@', 0x0f, 4},       /* setg  */
    {'=', '=', 0x04, 5},       /* sete  */
    {'!', '=', 0x05, 5},       /* setne */
    {'&', '&', 0x00, 9},       /* jump if zero     */
    {'|', '|', 0x01, 10},      /* jump if not zero */
    {'&', '@', 0xc821, 6},     /* and %ecx,%eax          */
    {'^', '@', 0xc831, 7},     /* xor %ecx,%eax          */
    {'|', '@', 0xc809, 8},     /* or %ecx,%eax           */
    {'~', '@', 0xd0f7, 2},     /* not %eax               */
    {'!', '@', 0x04, 2},       /* sete                   */
    {'*', '@', 0x00, 0},       /* pointer dereference    */
};

#define OPERATOR_COUNT (int)(sizeof(operators) / sizeof(operators[0]))

int token;            /* current token */
int token_value;      /* value of a number or operator token */
int token_level;      /* precedence level of an operator token */
int ch;               /* current input character */
int symbol_values;    /* symbol table: 8 bytes (value, extra) per name */
int return_jumps;     /* chain of jumps to the end of the current function */
int code_start;       /* start of the generated code buffer */
int code_ptr;         /* where the next instruction is written */
int local_offset;     /* stack space used by the local variables */
int data_ptr;         /* next free byte for globals and string literals */
int symbol_names;     /* space separated identifier names */
int symbol_names_end; /* end of symbol_names (also holds macro bodies) */
int macro_ptr;        /* where a macro body is read from, or 0 */
int macro_saved_ch;   /* input character saved while expanding a macro */
int last_identifier;  /* start of the last identifier name */
FILE *source;

void parse_expr();
int parse_test_expr();
void parse_declarations(int is_local);

void store_symbol_char(int c) { *(char *)symbol_names_end++ = c; }

/* read the next character from the source file or a macro body */
void read_char() {
  if (macro_ptr) {
    ch = *(char *)macro_ptr++;
    if (ch == MACRO_END) {
      macro_ptr = 0;
      ch = macro_saved_ch;
    }
  } else
    ch = fgetc(source);
}

int is_identifier_char() { return isalnum(ch) | ch == '_'; }

/* only "\n" really is an escape, every other backslash just quotes */
void read_escape() {
  if (ch == '\\') {
    read_char();
    if (ch == 'n')
      ch = '\n';
  }
}

void next_token() {
  int i, symbol;

  /* skip white space and preprocessor lines */
  while (isspace(ch) | ch == '#') {
    if (ch == '#') {
      read_char();
      next_token();
      if (token == TOK_DEFINE) {
        next_token();
        store_symbol_char(' ');
        *(int *)token = SYM_DEFINE;
        *(int *)(token + 4) = symbol_names_end; /* macro body */
      }
      /* the rest of the line is saved for a #define, harmless otherwise */
      while (ch != '\n') {
        store_symbol_char(ch);
        read_char();
      }
      store_symbol_char(ch);
      store_symbol_char(MACRO_END);
    }
    read_char();
  }

  token_level = 0;
  token = ch;

  if (is_identifier_char()) {
    store_symbol_char(' ');
    last_identifier = symbol_names_end;
    while (is_identifier_char()) {
      store_symbol_char(ch);
      read_char();
    }
    if (isdigit(token)) {
      token_value = strtol((char *)last_identifier, 0, 0);
      token = TOK_NUM;
    } else {
      /* look up " name " to find the first place the name was stored */
      *(char *)symbol_names_end = ' ';
      token = strstr((char *)symbol_names, (char *)last_identifier - 1) -
              (char *)symbol_names;
      *(char *)symbol_names_end = 0;
      token = token * 8 + TOK_IDENT;
      if (token > TOK_DEFINE) {
        /* a user identifier: the token becomes its symbol table slot */
        token = symbol_values + token;
        if (*(int *)token == SYM_DEFINE) {
          /* expand the macro by reading from its stored body */
          macro_ptr = *(int *)(token + 4);
          macro_saved_ch = ch;
          read_char();
          next_token();
        }
      }
    }
  } else {
    read_char();
    if (token == '\'') {
      token = TOK_NUM;
      read_escape();
      token_value = ch;
      read_char();
      read_char();
    } else if (token == '/' & ch == '*') {
      read_char();
      while (ch) {
        while (ch != '*')
          read_char();
        read_char();
        if (ch == '/')
          ch = 0;
      }
      read_char();
      next_token();
    } else {
      /* unknown characters end up with value 0 and level 0 */
      token_value = 0;
      for (i = 0; i < OPERATOR_COUNT; i++) {
        symbol = operators[i].second;
        token_value = operators[i].code;
        token_level = operators[i].level;
        if (operators[i].first == token & (symbol == ch | symbol == '@')) {
          if (symbol == ch) {
            read_char();
            token = TOK_DUMMY;
          }
          break;
        }
      }
    }
  }
}

/* write the non-zero low bytes of an instruction */
void emit(int bytes) {
  while (bytes && bytes != -1) {
    *(char *)code_ptr++ = bytes;
    bytes = bytes >> 8;
  }
}

/* fill in every jump of a chain so it lands on the current position */
void patch_jumps(int chain) {
  int next;
  while (chain) {
    next = *(int *)chain;
    *(int *)chain = code_ptr - chain - 4;
    chain = next;
  }
}

/* emit an instruction followed by a 32 bit value; returns where it is */
int emit_with_int(int opcode, int value) {
  emit(opcode);
  *(int *)code_ptr = value;
  value = code_ptr;
  code_ptr = code_ptr + 4;
  return value;
}

void emit_load_immediate(int value) { emit_with_int(OP_MOV_IMM_EAX, value); }

int emit_jump(int target) { return emit_with_int(OP_JMP, target); }

/* jump if %eax is zero (or not zero when jump_if_true is 1) */
int emit_test_jump(int jump_if_true, int target) {
  emit(OP_TEST_EAX_JCC);
  return emit_with_int(OP_JE + jump_if_true, target);
}

/* %eax = (%ecx <condition> %eax) */
void emit_compare(int condition) {
  emit(OP_CMP_EAX_ECX);
  emit_load_immediate(0);
  emit(OP_SETCC);
  emit(condition + OP_SETCC_BASE);
  emit(OP_MODRM_AL);
}

/* access a local (%ebp relative) or global (absolute) variable */
void emit_variable_access(int access, int address) {
  emit(access + OP_RM_ADD);
  emit_with_int((address < LOCAL_LIMIT) << 7 | 5, address);
}

/* allow_assign is 1 when a plain variable may be assigned with '=' */
void parse_unary(int allow_assign) {
  int kind, op, value, level, cast_type, arg_offset;

  kind = 1; /* 0 = forward function, 1 = value in %eax, else address */
  if (token == '"') {
    emit_load_immediate(data_ptr);
    while (ch != '"') {
      read_escape();
      *(char *)data_ptr++ = ch;
      read_char();
    }
    *(char *)data_ptr = 0;
    data_ptr = data_ptr + 4 & -4;
    read_char();
    next_token();
  } else {
    level = token_level;
    value = token_value;
    op = token;
    next_token();
    if (op == TOK_NUM) {
      emit_load_immediate(value);
    } else if (level == LEVEL_UNARY) {
      /* - + ! ~ : apply the operator to (0, operand) */
      parse_unary(0);
      emit_with_int(OP_MOV_IMM_ECX, 0);
      if (op == '!')
        emit_compare(value);
      else
        emit(value);
    } else if (op == '(') {
      parse_expr();
      next_token();
    } else if (op == '*') {
      /* dereference with a cast: (int *), (char *) or (int (*)()) */
      next_token();
      cast_type = token;
      next_token();
      next_token();
      if (token == '*') {
        next_token();
        next_token();
        next_token();
        next_token();
        cast_type = 0; /* function pointer */
      }
      next_token();
      parse_unary(0);
      if (token == '=') {
        next_token();
        emit(OP_PUSH_EAX);
        parse_expr();
        emit(OP_POP_ECX);
        emit(OP_STORE_ECX + (cast_type == TOK_INT));
      } else if (cast_type) {
        if (cast_type == TOK_INT)
          emit(OP_LOAD_INT);
        else
          emit(OP_LOAD_CHAR);
        code_ptr++; /* zero byte: (%eax) addressing */
      }
    } else if (op == '&') {
      emit_variable_access(ACCESS_ADDRESS, *(int *)token);
      next_token();
    } else {
      kind = *(int *)op;
      if (!kind)
        kind = dlsym(0, (char *)last_identifier);
      if (token == '=' & allow_assign) {
        next_token();
        parse_expr();
        emit_variable_access(ACCESS_STORE, kind);
      } else if (token != '(') {
        emit_variable_access(ACCESS_LOAD, kind);
        if (token_level == LEVEL_POSTFIX) {
          emit_variable_access(ACCESS_ADD_IMM8, kind);
          emit(token_value);
          next_token();
        }
      }
    }
  }

  /* function call */
  if (token == '(') {
    if (kind == 1)
      emit(OP_PUSH_EAX); /* save the function pointer */
    value = emit_with_int(OP_SUB_ESP, 0);
    next_token();
    arg_offset = 0;
    while (token != ')') {
      parse_expr();
      emit_with_int(OP_STORE_ARG, arg_offset);
      if (token == ',')
        next_token();
      arg_offset = arg_offset + 4;
    }
    *(int *)value = arg_offset;
    next_token();
    if (!kind) {
      /* not defined yet: add to the forward call chain of the symbol */
      op = op + 4;
      *(int *)op = emit_with_int(OP_CALL, *(int *)op);
    } else if (kind == 1) {
      emit_with_int(OP_CALL_ARG, arg_offset);
      arg_offset = arg_offset + 4;
    } else {
      emit_with_int(OP_CALL, kind - code_ptr - 5);
    }
    if (arg_offset)
      emit_with_int(OP_ADD_ESP, arg_offset);
  }
}

/* parse binary operators of the given precedence level and below */
void parse_binary(int level) {
  int op, value, jumps;

  if (level-- == 1)
    parse_unary(1);
  else {
    parse_binary(level);
    jumps = 0;
    while (level == token_level) {
      op = token;
      value = token_value;
      next_token();
      if (level > LEVEL_LOGICAL) {
        /* && and || : short circuit jump */
        jumps = emit_test_jump(value, jumps);
        parse_binary(level);
      } else {
        emit(OP_PUSH_EAX);
        parse_binary(level);
        emit(OP_POP_ECX);
        if (level == LEVEL_COMPARE | level == LEVEL_EQUALITY) {
          emit_compare(value);
        } else {
          emit(value);
          if (op == '%')
            emit(OP_XCHG_EDX_EAX);
        }
      }
    }
    if (jumps && level > LEVEL_LOGICAL) {
      jumps = emit_test_jump(value, jumps);
      emit_load_immediate(value ^ 1);
      emit_jump(5);
      patch_jumps(jumps);
      emit_load_immediate(value);
    }
  }
}

void parse_expr() { parse_binary(LEVEL_TOP); }

int parse_test_expr() {
  parse_expr();
  return emit_test_jump(0, 0);
}

/* break_chain points at the chain of 'break' jumps of the enclosing loop */
void parse_statement(int break_chain) {
  int exit_jumps, jump, keyword, step_jump;

  if (token == TOK_IF) {
    next_token();
    next_token();
    exit_jumps = parse_test_expr();
    next_token();
    parse_statement(break_chain);
    if (token == TOK_ELSE) {
      next_token();
      jump = emit_jump(0);
      patch_jumps(exit_jumps);
      parse_statement(break_chain);
      patch_jumps(jump);
    } else {
      patch_jumps(exit_jumps);
    }
  } else if (token == TOK_WHILE | token == TOK_FOR) {
    keyword = token;
    next_token();
    next_token();
    if (keyword == TOK_WHILE) {
      jump = code_ptr; /* loop start */
      exit_jumps = parse_test_expr();
    } else {
      if (token != ';')
        parse_expr();
      next_token();
      jump = code_ptr;
      exit_jumps = 0;
      if (token != ';')
        exit_jumps = parse_test_expr();
      next_token();
      if (token != ')') {
        /* the step expression is placed here and jumped over */
        step_jump = emit_jump(0);
        parse_expr();
        emit_jump(jump - code_ptr - 5);
        patch_jumps(step_jump);
        jump = step_jump + 4;
      }
    }
    next_token();
    parse_statement(&exit_jumps);
    emit_jump(jump - code_ptr - 5);
    patch_jumps(exit_jumps);
  } else if (token == '{') {
    next_token();
    parse_declarations(1);
    while (token != '}')
      parse_statement(break_chain);
    next_token();
  } else {
    if (token == TOK_RETURN) {
      next_token();
      if (token != ';')
        parse_expr();
      return_jumps = emit_jump(return_jumps);
    } else if (token == TOK_BREAK) {
      next_token();
      *(int *)break_chain = emit_jump(*(int *)break_chain);
    } else if (token != ';')
      parse_expr();
    next_token();
  }
}

/* global (is_local = 0) or local (is_local = 1) declarations */
void parse_declarations(int is_local) {
  int offset;

  while (token == TOK_INT | token != -1 & !is_local) {
    if (token == TOK_INT) {
      next_token();
      while (token != ';') {
        if (is_local) {
          local_offset = local_offset + 4;
          *(int *)token = -local_offset;
        } else {
          *(int *)token = data_ptr;
          data_ptr = data_ptr + 4;
        }
        next_token();
        if (token == ',')
          next_token();
      }
      next_token();
    } else {
      /* function definition: fix the calls made before it was defined */
      patch_jumps(*(int *)(token + 4));
      *(int *)token = code_ptr;
      next_token();
      next_token();
      offset = 8; /* the first argument is at 8(%ebp) */
      while (token != ')') {
        *(int *)token = offset;
        offset = offset + 4;
        next_token();
        if (token == ',')
          next_token();
      }
      next_token();
      return_jumps = local_offset = 0;
      emit(OP_PROLOGUE);
      offset = emit_with_int(OP_SUB_ESP, 0);
      parse_statement(0);
      patch_jumps(return_jumps);
      emit(OP_EPILOGUE);
      *(int *)offset = local_offset;
    }
  }
}

int allocate() {
  return mmap(NULL, BUFFER_SIZE, PROT_EXEC | PROT_WRITE | PROT_READ,
              MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
}

int main(int argc, char **argv) {
  source = stdin;
  if (argc-- > 1) {
    /* the compiled program sees the source file as argv[0] */
    *argv = argv[1];
    source = fopen(*argv, "r");
  }
  symbol_names = allocate();
  symbol_names_end = strcpy((char *)symbol_names, KEYWORDS) + KEYWORDS_LENGTH;
  data_ptr = allocate();
  code_ptr = code_start = allocate();
  symbol_values = allocate();
  read_char();
  next_token();
  parse_declarations(0);
  return (*(int (*)()) * (int *)(symbol_values + TOK_MAIN))(argc, argv);
}
