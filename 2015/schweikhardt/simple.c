#include <stdio.h>
#include <stdlib.h>
#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
#include <stdint.h>
#endif

/*
 * Numbers are stored as an array of limbs, least significant limb first.
 * The limb type is chosen when compiling with -Dtyp=... (the Makefile uses
 * uint64_t); it must be an unsigned integer type.
 */
#ifdef typ
typedef typ limb_t;
#else
typedef uint64_t limb_t;
#endif

typedef struct {
	size_t length;
	limb_t *limbs;
} bignum;

#define BITS_PER_HEX_DIGIT 4
#define HEX_DIGIT_MASK 0xFu
#define LIMB_BITS (sizeof(limb_t) * 8)
#define HEX_DIGITS_PER_LIMB (LIMB_BITS / BITS_PER_HEX_DIGIT)
#define LIMB_MAX ((limb_t)~(limb_t)0)
#define LIMB_TOP_BIT ((limb_t)((limb_t)1 << (LIMB_BITS - 1)))

/* The exit status is whatever puts() returned, as in the original. */
static void die(const char *message)
{
	exit(puts(message));
}

/* Print every limb as zero padded upper case hex, most significant first. */
static void print_hex(const bignum *number)
{
	size_t index = number->length;

	do {
		int shift = (int)LIMB_BITS - BITS_PER_HEX_DIGIT;

		--index;
		while (shift >= 0) {
			printf("%X", (unsigned)((number->limbs[index] >> shift) & HEX_DIGIT_MASK));
			shift -= BITS_PER_HEX_DIGIT;
		}
	} while (index != 0);
}

/* Add one more significant limb holding the given value. */
static void append_limb(bignum *number, limb_t value)
{
	limb_t *grown = realloc(number->limbs, (number->length + 1) * sizeof(limb_t));

	if (grown == NULL) {
		free(number->limbs);
		die("throw up");
	}
	number->limbs = grown;
	number->limbs[number->length] = value;
	++number->length;
}

/*
 * Letters are moved up by 9 so that 'a' and 'A' end in the bits for 10. Any
 * other character is accepted too and simply contributes its low four bits.
 */
static unsigned hex_digit_value(unsigned char character)
{
	unsigned value = character;

	if (character > '@') {
		value += 9;
	}
	return value & HEX_DIGIT_MASK;
}

/* Returns 0 if there is no memory for the number. */
static int parse_hex(const char *text, size_t digit_count, bignum *number)
{
	size_t limb_index = 0;
	size_t shift = 0;
	size_t position;

	number->length = (digit_count + HEX_DIGITS_PER_LIMB - 1) / HEX_DIGITS_PER_LIMB;
	number->limbs = calloc(number->length, sizeof(limb_t));
	if (number->limbs == NULL) {
		return 0;
	}

	/* The last character of the text is the least significant digit. */
	for (position = digit_count; position > 0; --position) {
		limb_t digit = hex_digit_value((unsigned char)text[position - 1]);

		number->limbs[limb_index] |= (limb_t)(digit << shift);
		shift += BITS_PER_HEX_DIGIT;
		if (shift == LIMB_BITS) {
			shift = 0;
			++limb_index;
		}
	}
	return 1;
}

/* True for 1, and also for 0, which would otherwise never finish. */
static int is_finished(const bignum *number)
{
	size_t index;

	if (number->limbs[0] > 1) {
		return 0;
	}
	for (index = 1; index < number->length; ++index) {
		if (number->limbs[index] != 0) {
			return 0;
		}
	}
	return 1;
}

static int is_odd(const bignum *number)
{
	return (number->limbs[0] & 1) != 0;
}

static bignum copy_of(bignum *number)
{
	bignum copy;
	size_t index;

	copy.length = number->length;
	copy.limbs = malloc(copy.length * sizeof(limb_t));
	if (copy.limbs == NULL) {
		free(number->limbs);
		die("laugh");
	}
	for (index = 0; index < copy.length; ++index) {
		copy.limbs[index] = number->limbs[index];
	}
	return copy;
}

static void shift_left_one(bignum *number)
{
	limb_t carry = 0;
	size_t index;

	if (number->limbs[number->length - 1] & LIMB_TOP_BIT) {
		append_limb(number, 0);
	}
	for (index = 0; index < number->length; ++index) {
		limb_t next_carry = (number->limbs[index] & LIMB_TOP_BIT) ? 1 : 0;

		number->limbs[index] = (limb_t)((limb_t)(number->limbs[index] << 1) + carry);
		carry = next_carry;
	}
}

static void shift_right_one(bignum *number)
{
	size_t index;

	number->limbs[0] = (limb_t)(number->limbs[0] >> 1);
	for (index = 1; index < number->length; ++index) {
		if (number->limbs[index] & 1) {
			number->limbs[index - 1] |= LIMB_TOP_BIT;
		}
		number->limbs[index] = (limb_t)(number->limbs[index] >> 1);
	}
}

/* Unsigned addition wraps around, so overflow is tested before adding. */
static void add(bignum *sum, bignum *addend)
{
	limb_t carry = 0;
	size_t index;

	while (addend->length < sum->length) {
		append_limb(addend, 0);
	}
	for (index = 0; index < sum->length; ++index) {
		limb_t room = (limb_t)(LIMB_MAX - addend->limbs[index]);

		if (carry) {
			carry = (sum->limbs[index] >= room) ? 1 : 0;
			++sum->limbs[index];
		} else {
			carry = (sum->limbs[index] > room) ? 1 : 0;
		}
		sum->limbs[index] = (limb_t)(sum->limbs[index] + addend->limbs[index]);
	}
	if (carry) {
		append_limb(sum, 1);
	}
}

static void increment(bignum *number)
{
	size_t index;

	for (index = 0; index < number->length; ++index) {
		++number->limbs[index];
		if (number->limbs[index] != 0) {
			return;
		}
	}
	append_limb(number, 1);
}

/* n = 2n + n + 1 */
static void triple_plus_one(bignum *number)
{
	bignum original = copy_of(number);

	shift_left_one(number);
	add(number, &original);
	increment(number);
	free(original.limbs);
}

/* The number never loses limbs, so leading zero limbs stay in the output. */
static void print_collatz_sequence(bignum *number)
{
	int step = 1;

	print_hex(number);
	printf("\n");
	while (!is_finished(number)) {
		if (is_odd(number)) {
			triple_plus_one(number);
		} else {
			shift_right_one(number);
		}
		print_hex(number);
		printf(" %d\n", step);
		++step;
	}
}

int main(int argc, char *argv[])
{
	if (argc == 2) {
		const char *text = argv[1];
		size_t digit_count = 0;

		while (text[digit_count] != '\0') {
			++digit_count;
		}
		if (digit_count != 0) {
			bignum number;

			if (parse_hex(text, digit_count, &number)) {
				print_collatz_sequence(&number);
				free(number.limbs);
			}
		}
	}
	return fclose(stdout);
}
