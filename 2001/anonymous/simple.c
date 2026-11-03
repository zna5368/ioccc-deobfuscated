/*
 * 2001/anonymous - a dynamic binary translator for 32-bit x86 ELF programs
 *
 * The translator decodes one basic block of x86 code, turns it into C,
 * recompiles THIS source file with that C code baked in (via -D options),
 * and executes the new program.  The x86 registers are kept inside the ELF
 * file itself (it is mapped shared and writable) so that they survive from
 * one compiled block to the next.
 */
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/stat.h>

/*
 * When a block has been translated this file is recompiled with
 *
 *     -DCOMPILER=AS_STRING(<compiler command>)
 *     -DTRANSLATED_BLOCK=<C code for the block>,AS_STRING
 *
 * In the recompiled program the start-up line
 *     TRANSLATED_BLOCK(start_at_main(size));
 * becomes
 *     <C code for the block>, AS_STRING(start_at_main(size));
 * so the block runs and the one-time start-up call is turned into a
 * harmless string literal.
 */
#define AS_STRING(text) #text

#ifndef COMPILER
#define COMPILER "cc"
#endif

#ifndef TRANSLATED_BLOCK
#define TRANSLATED_BLOCK /* first run: nothing translated yet */
#endif

#define OPEN_READ_WRITE 2 /* O_RDWR */

/* 32-bit words of the ELF header */
#define ELF_SECTION_TABLE_OFFSET 8 /* e_shoff */
#define ELF_SECTION_COUNT 12       /* e_shnum | e_shstrndx << 16 */

/* fields of a 40 byte (10 word) section header */
#define SH_NAME 0
#define SH_TYPE 1
#define SH_ADDR 3
#define SH_OFFSET 4
#define SH_SIZE 5
#define SECTION_HEADER_WORDS 10
#define SHT_SYMTAB 2
#define SYMBOL_BYTES 16 /* st_name, st_value, st_size, st_info... */

/*
 * The eight x86 registers (eax, ecx, edx, ebx, esp, ebp, esi, edi) are
 * stored in the mapped file right after the 52 byte ELF header.
 */
#define REG (image + 13)
#define EAX 0
#define ESP 4

/* x86 stack operations used by the generated code */
#define PUSH(value)                                                            \
  (REG[ESP] = le32(le32(REG[ESP]) - 4), *(int *)le32(REG[ESP]) = (value))
#define POP()                                                                  \
  (popped = *(int *)le32(REG[ESP]), REG[ESP] = le32(le32(REG[ESP]) + 4), popped)

/* x86 opcodes understood by the translator */
#define OP_XOR 0x31       /* xor r/m32, r32 (only "xor reg,reg") */
#define OP_DEC_FIRST 0x40 /* 0x40-0x4f inc/dec reg (both decrement!) */
#define OP_DEC_LAST 0x4f
#define OP_PUSH_FIRST 0x50 /* 0x50-0x57 push reg */
#define OP_PUSH_LAST 0x57
#define OP_POP_FIRST 0x58 /* 0x58-0x5f pop reg */
#define OP_POP_LAST 0x5f
#define OP_PUSH_IMM 0x68    /* push imm32 */
#define OP_JNE_SHORT 0x75   /* jne rel8 (follows a cmp) */
#define OP_GROUP_IMM32 0x81 /* add/sub/cmp r/m32, imm32 */
#define OP_GROUP_IMM8 0x83  /* add/sub/cmp r/m32, imm8 */
#define OP_TEST 0x85        /* test r/m32, r32 */
#define OP_MOV_STORE 0x89   /* mov r/m32, r32 */
#define OP_MOV_LOAD 0x8b    /* mov r32, r/m32 */
#define OP_LEA 0x8d         /* lea r32, m */
#define OP_MOV_EAX_IMM 0xb8 /* mov eax, imm32 */
#define OP_RET 0xc3
#define OP_CALL 0xe8      /* call rel32 */
#define OP_JMP_SHORT 0xeb /* jmp rel8 */
#define OP_GROUP_FF 0xff  /* inc/dec/jmp/push r/m32 */

struct stat file_info;
off_t image_size;

char block_program[99]; /* name of the compiled program for the block */
char operand[99];       /* C text of the current r/m operand */
char command[9999];     /* compiler command line + the translated block */
char *command_end = command;
char *strtab;       /* the .strtab section (symbol names) */
char *endian_probe; /* first byte of a non-zero pointer variable;
                       NULL until the program has started up */

int *image;         /* the x86 ELF file, mapped shared */
int symtab_section; /* index of the .symtab section header */
int popped;         /* temporary used by POP() */
int opcode;         /* current (or previous) opcode */
int pc;             /* x86 instruction pointer */
int fd;

/* Convert between the file's little-endian words and host words. */
int le32(int value) {
  if (*endian_probe) /* little-endian host */
    return value;
  return value << 24 | (value & 0xff00) << 8 | value >> 8 & 0xff00 |
         value >> 24 & 0xff;
}

int image_word(int index) { return le32(image[index]); }

int section_header(int index, int field) {
  return le32(image[image_word(ELF_SECTION_TABLE_OFFSET) / 4 +
                    index * SECTION_HEADER_WORDS + field]);
}

int section_count(void) { return image_word(ELF_SECTION_COUNT) & 63; }

/*
 * Convert an x86 address into a host pointer by looking for the section
 * that contains it.  Addresses that are already near the mapped file (the
 * stack) are returned unchanged.
 */
int *guest_ptr_from(int address, int index) {
  int start = section_header(index, SH_ADDR);

  if (start && start <= address)
    return (int *)((int)image + address + section_header(index, SH_OFFSET) -
                   start);
  if ((((int)image) ^ address) >> 24 && --index)
    return guest_ptr_from(address, index);
  return (int *)address;
}

int *guest_ptr(int address) { return guest_ptr_from(address, section_count()); }

/* the n-th 32-bit argument on the x86 stack, as a host pointer */
int *stack_arg(int n) { return guest_ptr(le32(((int *)le32(REG[ESP]))[n])); }

/* Locate the .symtab section and the .strtab section of symbol names. */
void find_symbol_tables(void) {
  char *section_names =
      (char *)image +
      section_header(image_word(ELF_SECTION_COUNT) >> 16, SH_OFFSET);

  symtab_section = section_count();
  do {
    --symtab_section;
    if (!strcmp(section_names + section_header(symtab_section, SH_NAME),
                ".strtab"))
      strtab = (char *)image + section_header(symtab_section, SH_OFFSET);
  } while (section_header(symtab_section, SH_TYPE) != SHT_SYMTAB);
}

/* Search the symbol table from the end; return the symbol's word index. */
int find_symbol_named(char *name) {
  int offset = section_header(symtab_section, SH_SIZE);
  int symbol;

  do {
    offset -= SYMBOL_BYTES;
    symbol = (offset + section_header(symtab_section, SH_OFFSET)) / 4;
  } while (strcmp(strtab + image_word(symbol), name));
  return symbol;
}

int find_symbol_at(int address) {
  int offset = section_header(symtab_section, SH_SIZE);
  int symbol;

  do {
    offset -= SYMBOL_BYTES;
    symbol = (offset + section_header(symtab_section, SH_OFFSET)) / 4;
  } while (image_word(symbol + 1) != address);
  return symbol;
}

/* read x86 code at pc */
int fetch_byte(void) { return (signed char)le32(*guest_ptr(pc++)); }

int fetch_word(void) {
  pc += 4;
  return *guest_ptr(pc - 4);
}

/*
 * The block is finished: close the -DTRANSLATED_BLOCK=' option and
 * recompile this file with it.
 */
void compile_block(void) {
  sprintf(command_end, "AS_STRING'");
  system(command);
}

/* First run: map the x86 program and prepare to translate main(). */
void start_at_main(int size) {
  REG[ESP] = le32((int)(image + size / 4)); /* stack at the end of the file */
  pc = image_word(find_symbol_named("main") + 1);
  REG[ESP] = le32(le32(REG[ESP]) - 4); /* room for main's return address */
}

void start_up(char **argv) {
  int size;

  stat(argv[1], &file_info);
  size = image_size = file_info.st_size;
  fd = open(argv[1], OPEN_READ_WRITE);
  image = (int *)mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  find_symbol_tables();

  TRANSLATED_BLOCK(start_at_main(size));

  /* the compiled program for the block starting at pc */
  sprintf(block_program, ".%s%d", argv[1], pc);
  fflush(NULL);
  command_end += sprintf(command_end,
                         "%s %s '-DCOMPILER=AS_STRING(%s)' -o %s "
                         "'-DTRANSLATED_BLOCK=",
                         COMPILER, __FILE__, COMPILER, block_program);
}

/*
 * Instructions 0x81, 0x83 and 0xff: the reg field of the ModRM byte picks
 * the operation.
 */
void translate_group(int reg_field) {
  int value = 0, relative;
  char *name, *at, compare;

  if (reg_field == 4) {
    /* jmp *addr: a PLT stub.  Find its symbol, e.g. "printf@GLIBC_2.0" */
    name = strtab + image_word(find_symbol_at(pc - 2));
    at = index(name, '@');
    if (at)
      *at = 0;
  } else if (reg_field != 1 && reg_field != 6) {
    value = opcode == OP_GROUP_IMM32 ? fetch_word() : fetch_byte();
  }

  switch (reg_field) {
  case 0: /* add */
  case 5: /* sub */
    command_end += sprintf(command_end, "%s=le32(le32(%s)%c%d),", operand,
                           operand, reg_field ? '-' : '+', value);
    return;
  case 1: /* dec */
    command_end +=
        sprintf(command_end, "%s=le32(le32(%s)-1),", operand, operand);
    return;
  case 6: /* push */
    command_end += sprintf(command_end, "PUSH(%s),", operand);
    return;
  }

  /* cmp (reg 7) followed by je/jne: compare and jump are done together */
  compare = fetch_byte() == OP_JNE_SHORT ? '=' : '!';
  relative = fetch_byte(); /* (also read, but unused, for the call) */
  if (reg_field == 4) {
    /* call the host library function with three stack arguments */
    command_end +=
        sprintf(command_end,
                "REG[EAX]=le32(((int(*)(int*,int*,int*))%s)"
                "(stack_arg(1),stack_arg(2),stack_arg(3))),pc=POP(),",
                name);
  } else {
    command_end += sprintf(command_end, "pc=%d%c=%s?%d:%d,", value, compare,
                           operand, pc, pc + relative);
  }
  compile_block();
}

/* Translate one x86 instruction at pc into C. */
void translate_instruction(char **argv) {
  int after_opcode, modrm, reg_field, rm_field, relative;

  /* basic block cache: run the block if it has been compiled already */
  if (stat(block_program, &file_info) == 0)
    execv(block_program, argv);
  else if (opcode == OP_TEST)
    pc += 3; /* the 3 bytes after a test instruction are skipped */

  opcode = fetch_byte() & 255;
  after_opcode = pc;
  modrm = fetch_byte() & 255;
  reg_field = modrm >> 3 & 7;
  rm_field = modrm & 7;
  if (modrm >> 6 == 3)
    sprintf(operand, "REG[%d]", rm_field);
  else
    sprintf(operand, "*guest_ptr(le32(REG[%d])+%d)", rm_field,
            modrm >> 6 == 1 ? fetch_byte() : 0);

  if (opcode == OP_GROUP_IMM32 || opcode == OP_GROUP_IMM8 ||
      opcode == OP_GROUP_FF) {
    translate_group(reg_field);
    return;
  }

  if (opcode == OP_RET || opcode == OP_CALL || opcode == OP_JMP_SHORT) {
    pc = after_opcode; /* no ModRM byte */
    if (opcode == OP_RET) {
      command_end += sprintf(command_end, "pc=POP(),");
    } else if (opcode == OP_CALL) {
      relative = le32(fetch_word());
      command_end += sprintf(command_end, "PUSH(%d),pc=%d,", pc, pc + relative);
    } else {
      relative = fetch_byte();
      command_end += sprintf(command_end, "pc=%d,", pc + relative);
    }
    compile_block();
    return;
  }

  if (opcode == OP_XOR) {
    command_end += sprintf(command_end, "REG[%d]=0,", reg_field);
  } else if (opcode == OP_TEST) {
    command_end +=
        sprintf(command_end, "REG[EAX]=%s==REG[%d],", operand, reg_field);
  } else if (opcode == OP_MOV_STORE) {
    command_end += sprintf(command_end, "%s=REG[%d],", operand, reg_field);
  } else if (opcode == OP_MOV_LOAD) {
    command_end += sprintf(command_end, "REG[%d]=(int)%s,", reg_field, operand);
  } else if (opcode == OP_LEA) {
    command_end +=
        sprintf(command_end, "REG[%d]=(int)&%s,", reg_field, operand);
  } else {
    pc = after_opcode; /* the rest have no ModRM byte */
    if (opcode >= OP_DEC_FIRST && opcode <= OP_DEC_LAST)
      command_end += sprintf(command_end, "REG[%d]=le32(le32(REG[%d])-1),",
                             opcode & 7, opcode & 7);
    else if (opcode >= OP_PUSH_FIRST && opcode <= OP_PUSH_LAST)
      command_end += sprintf(command_end, "PUSH(REG[%d]),", opcode & 7);
    else if (opcode >= OP_POP_FIRST && opcode <= OP_POP_LAST)
      command_end += sprintf(command_end, "REG[%d]=POP(),", opcode & 7);
    else if (opcode == OP_PUSH_IMM)
      command_end += sprintf(command_end, "PUSH(%d),", fetch_word());
    else if (opcode == OP_MOV_EAX_IMM)
      command_end += sprintf(command_end, "REG[EAX]=%d,", fetch_word());
    /* anything else is not supported */
  }
}

void translate_step(char *nonzero, char **argv) {
  if (endian_probe) {
    translate_instruction(argv);
  } else {
    /*
     * The first byte of a pointer holding a small non-zero number is
     * non-zero only on a little-endian machine.
     */
    endian_probe = (char *)&nonzero;
    start_up(argv);
  }
}

/*
 * In this version of the entry main() only does the start-up step (which
 * writes the stack pointer into the file) and then runs the program natively.
 */
int main(int argc, char **argv) {
  if (argc)
    translate_step((char *)argc, argv);
  else /* remove the compiled block programs */
    system((sprintf(block_program, "rm -f .%s", argv[1]), block_program));

  munmap(image, image_size);
  close(fd);
  execv(argv[1], argv);
}
