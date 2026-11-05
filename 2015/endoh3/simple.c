#include <unistd.h>

/*
 * A tiny PDP-11 emulator.
 *
 * The program that the emulator runs is whatever the linker places at the
 * symbol "main".  Link this file with a source that defines an array such as
 * `short main[] = { ... };` and that array becomes the PDP-11 memory image.
 * Without one, memory is all zero and the first instruction is rejected.
 */

typedef unsigned short Word;

/* The two registers that the instructions name directly */
#define R0 registers[0]
#define PC registers[7]

/* Bit fields of an instruction word; octal lines up with the PDP-11 manuals */
#define SOURCE_FIELD(instruction) ((instruction) >> 6)
#define DESTINATION_FIELD(instruction) (instruction)
#define TRAP_NUMBER(instruction) ((instruction) % 64)
#define SOB_REGISTER(instruction) ((instruction) >> 6 & 7)
#define SOB_OFFSET(instruction) ((instruction) % 64)

/* Instruction patterns */
#define HALT 0
#define IS_BR(instruction) ((instruction) >> 8 == 01)
#define IS_TRAP(instruction) ((instruction) >> 6 == 01044)
#define IS_SOB(instruction) ((instruction) >> 9 == 077)
#define IS_TST(instruction) ((instruction) >> 6 == 057)
#define IS_MOV(instruction) ((instruction) >> 12 == 01)
#define IS_SUB(instruction) ((instruction) >> 12 == 016)

/* Addressing modes */
#define MODE_REGISTER 0
#define MODE_REGISTER_DEFERRED 1
#define MODE_AUTO_DECREMENT 4
#define MODE_INDEX 6

/* The only system call: write(R0, buffer, length) with inline arguments */
#define TRAP_WRITE 4
#define TRAP_ARGUMENTS_SIZE 4
#define TRAP_DELAY_MICROSECONDS 10

#define STDERR 2

/* 64 KiB of memory, replaced by the definition of main in the linked program */
Word main[32768];
Word registers[8];

/* Returns a pointer to the word at a byte address of the emulated memory */
static Word *memory_at(int address)
{
	return (Word *)((char *)main + address);
}

/*
 * Decodes the six bit operand in the low bits of field, performing any
 * register update required by the addressing mode, and returns where the
 * operand lives.
 *
 * Every mode that is not listed behaves as auto-increment.  Index mode always
 * steps the register over the index word, so it only works with the PC.
 */
static Word *operand_address(int field)
{
	Word *reg = &registers[field & 7];

	switch (field >> 3 & 7) {
	case MODE_REGISTER:
		return reg;
	case MODE_REGISTER_DEFERRED:
		return memory_at(*reg);
	case MODE_AUTO_DECREMENT:
		*reg -= 2;
		return memory_at(*reg);
	case MODE_INDEX:
		*reg += 2;
		return memory_at(*reg + *memory_at(*reg - 2));
	default: /* auto-increment */
		*reg += 2;
		return memory_at(*reg - 2);
	}
}

static void illegal_instruction(void)
{
	static const char message[] = "Illegal instruction ;-(\n";

	_exit(write(STDERR, message, sizeof message - 1));
}

/* All traps are slowed down so that the animation can be seen */
static void trap(Word instruction)
{
	usleep(TRAP_DELAY_MICROSECONDS);

	if (TRAP_NUMBER(instruction) == TRAP_WRITE) {
		Word buffer = *memory_at(PC);
		Word length = *memory_at(PC + 2);

		write(R0, memory_at(buffer), length);
		PC += TRAP_ARGUMENTS_SIZE;
	}
}

/* Subtract one from a register and branch backwards unless it reached zero */
static void subtract_one_and_branch(Word instruction)
{
	Word *counter = &registers[SOB_REGISTER(instruction)];

	if (--*counter != 0) {
		PC -= SOB_OFFSET(instruction) * 2;
	}
}

/*
 * Both operand addresses are decoded before the source is read, so the source
 * sees any register update made by the destination, as in MOV R1,(R1)+.
 */
static void move(Word instruction)
{
	Word *source = operand_address(SOURCE_FIELD(instruction));
	Word *destination = operand_address(DESTINATION_FIELD(instruction));

	*destination = *source;
}

/* Unlike move, the source is read before the destination is decoded */
static void subtract(Word instruction)
{
	Word subtrahend = *operand_address(SOURCE_FIELD(instruction));
	Word *destination = operand_address(DESTINATION_FIELD(instruction));

	*destination -= subtrahend;
}

/*
 * The constructor attribute runs this function before main would be called.
 * It never returns, which is fortunate as main is not a function here.
 */
__attribute__((constructor)) void run_emulator(void)
{
	for (;;) {
		Word instruction = *memory_at(PC);

		PC += 2;

		if (instruction == HALT) {
			illegal_instruction();
		} else if (IS_BR(instruction)) {
			PC += (signed char)instruction * 2;
		} else if (IS_TRAP(instruction)) {
			trap(instruction);
		} else if (IS_SOB(instruction)) {
			subtract_one_and_branch(instruction);
		} else if (IS_TST(instruction)) {
			/* Flags are not emulated, only the addressing side effects */
			operand_address(DESTINATION_FIELD(instruction));
		} else if (IS_MOV(instruction)) {
			move(instruction);
		} else if (IS_SUB(instruction)) {
			subtract(instruction);
		}
		/* Every other instruction is silently ignored */
	}
}
