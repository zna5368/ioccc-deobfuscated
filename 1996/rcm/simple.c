#include <stdio.h>
#include <stdlib.h>

/*
 * A minimal gzip (RFC 1952) / deflate (RFC 1951) decompressor.
 * Reads a .gz file on stdin and writes the uncompressed data to stdout.
 * No CRC or length checking is done.
 */

#define WINDOW_SIZE 32768

/* order in which code length code lengths are stored (RFC 1951 3.2.7) */
unsigned code_length_order[] = {16, 17, 18, 0, 8,  7, 9,  6, 10, 5,
                                11, 4,  12, 3, 13, 2, 14, 1, 15};

/* base lengths for length symbols 257..285 */
unsigned length_base[] = {3,  4,  5,  6,  7,  8,  9,  10, 11,  13,
                          15, 17, 19, 23, 27, 31, 35, 43, 51,  59,
                          67, 83, 99, 115, 131, 163, 195, 227, 258};

/* extra bits for length symbols 257..285 */
unsigned length_extra[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                           2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};

/* base distances for distance symbols 0..29 */
unsigned dist_base[] = {1,    2,    3,    4,    5,    7,     9,     13,
                        17,   25,   33,   49,   65,   97,    129,   193,
                        257,  385,  513,  769,  1025, 1537,  2049,  3073,
                        4097, 6145, 8193, 12289, 16385, 24577};

/* extra bits for distance symbols 0..29 */
unsigned dist_extra[] = {0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
                         6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

unsigned bit_count;         /* number of valid bits in bit_buffer */
unsigned long bit_buffer;   /* bits read from input, least significant first */

unsigned code_lengths[320]; /* code length of each symbol */
unsigned length_count[17];  /* number of codes of each length */
unsigned next_code[17];     /* next canonical code of each length */

/* circular output window (also the history used by back references) */
unsigned char window[WINDOW_SIZE];
unsigned window_out; /* oldest byte not yet written to stdout */
unsigned window_in;  /* position where the next byte is stored */

/* binary Huffman tree node; leaves have no children */
struct node {
  struct node *zero, *one;
  unsigned symbol;
};

struct node empty_node;
struct node *literal_tree, *distance_tree;

/* read count bits from the input, least significant bit first */
unsigned read_bits(unsigned count) {
  unsigned value;

  if (!count)
    return 0;
  for (; bit_count < count; bit_count += 8)
    bit_buffer |= 0l + getchar() << bit_count;
  value = bit_buffer & (1 << count) - 1;
  bit_buffer >>= count;
  bit_count -= count;
  return value;
}

/* walk the Huffman tree one bit at a time until a leaf is reached */
unsigned decode_symbol(struct node *tree) {
  while (tree->zero)
    tree = read_bits(1) ? tree->one : tree->zero;
  return tree->symbol;
}

/* build a Huffman tree from code_lengths[0..count-1] (RFC 1951 3.2.2) */
struct node *build_tree(unsigned count) {
  struct node *tree = 0, **link;
  unsigned i, code, bits;

  for (i = 0; i < 17; i++)
    length_count[i] = 0;
  for (i = 0; i < count; i++)
    length_count[code_lengths[i]]++;

  length_count[0] = next_code[0] = 0;
  for (i = 0; i < 16; i++)
    next_code[i + 1] = (next_code[i] + length_count[i]) << 1;

  for (i = 0; i < count; i++) {
    if (!code_lengths[i])
      continue;
    code = next_code[code_lengths[i]]++;
    bits = code_lengths[i];

    /* follow the code from its most significant bit, creating nodes */
    link = &tree;
    while (bits--) {
      if (!*link) {
        *link = malloc(sizeof(struct node));
        **link = empty_node;
      }
      link = ((code >> bits) & 1) ? &(*link)->one : &(*link)->zero;
    }
    *link = malloc(sizeof(struct node));
    **link = empty_node;
    (*link)->symbol = i;
  }
  return tree;
}

void free_tree(struct node *tree) {
  struct node *one;

  if (tree) {
    free_tree(tree->zero);
    one = tree->one;
    free(tree);
    free_tree(one);
  }
}

/* store a byte in the window, writing out the oldest byte once it is full */
void put_byte(unsigned c) {
  window[window_in++] = c;
  if ((window_in %= WINDOW_SIZE) == (window_out %= WINDOW_SIZE))
    putchar(window[window_out++]);
}

/* block type 0: uncompressed data */
void stored_block(void) {
  unsigned length;

  if (bit_count % 8)
    read_bits(bit_count % 8); /* skip to a byte boundary */
  length = read_bits(16);
  read_bits(16); /* one's complement of length, ignored */
  while (length--)
    put_byte(read_bits(8));
}

/* block type 1: fixed Huffman codes */
void fixed_tables(void) {
  unsigned i;

  for (i = 0; i < 288; i++)
    code_lengths[i] = i < 144 ? 8 : i < 256 ? 9 : i < 280 ? 7 : 8;
  literal_tree = build_tree(288);
  for (i = 0; i < 32; i++)
    code_lengths[i] = 5;
  distance_tree = build_tree(32);
}

/* block type 2: dynamic Huffman codes */
void dynamic_tables(void) {
  unsigned literal_count = read_bits(5) + 257;
  unsigned distance_count = read_bits(5) + 1;
  unsigned length_code_count = read_bits(4) + 4;
  unsigned i, length, repeat, previous;
  struct node *length_tree;

  for (i = 0; i < length_code_count; i++)
    code_lengths[code_length_order[i]] = read_bits(3);
  for (; i < 19; i++)
    code_lengths[code_length_order[i]] = 0;
  length_tree = build_tree(19);

  for (previous = i = 0; i < literal_count + distance_count;) {
    length = decode_symbol(length_tree);
    if (length == 16) { /* repeat previous length 3-6 times */
      length = previous;
      repeat = 3 + read_bits(2);
    } else if (length == 17) { /* repeat zero 3-10 times */
      length = 0;
      repeat = 3 + read_bits(3);
    } else if (length == 18) { /* repeat zero 11-138 times */
      length = 0;
      repeat = 11 + read_bits(7);
    } else {
      previous = length;
      repeat = 1;
    }
    while (repeat--)
      code_lengths[i++] = length;
  }
  free_tree(length_tree);

  literal_tree = build_tree(literal_count);
  for (i = distance_count; i--;)
    code_lengths[i] = code_lengths[i + literal_count];
  distance_tree = build_tree(distance_count);
}

/* decode literals and back references until the end of block symbol */
void inflate_codes(void) {
  unsigned symbol, length, distance, from;

  while ((symbol = decode_symbol(literal_tree)) != 256) {
    if (symbol < 256) {
      put_byte(symbol);
      continue;
    }
    symbol -= 257;
    length = length_base[symbol] + read_bits(length_extra[symbol]);
    symbol = decode_symbol(distance_tree);
    distance = dist_base[symbol] + read_bits(dist_extra[symbol]);
    from = distance <= window_in ? window_in - distance
                                 : WINDOW_SIZE - distance + window_in;
    for (; length--; from %= WINDOW_SIZE)
      put_byte(window[from++]);
  }
}

main() {
  unsigned flags, extra_length, last_block, block_type;

  /* gzip header */
  read_bits(24); /* ID1, ID2, CM */
  flags = read_bits(8);
  read_bits(24); /* MTIME, XFL, OS */
  read_bits(24);
  if (flags & 4) /* FEXTRA */
    for (extra_length = read_bits(16); extra_length--;)
      read_bits(8);
  if (flags & 8) /* FNAME: zero terminated */
    while (read_bits(8))
      ;
  if (flags & 16) /* FCOMMENT: zero terminated */
    while (read_bits(8))
      ;
  if (flags & 2) /* FHCRC */
    read_bits(16);

  /* deflate blocks */
  for (last_block = 0; !last_block;) {
    last_block = read_bits(1);
    block_type = read_bits(2);
    if (!block_type) {
      stored_block();
      continue;
    }
    if (block_type & 1)
      fixed_tables();
    if (block_type & 2)
      dynamic_tables();
    inflate_codes();
    free_tree(literal_tree);
    free_tree(distance_tree);
  }

  /* flush whatever is left in the window */
  for (; (window_out %= WINDOW_SIZE) != (window_in %= WINDOW_SIZE);
       putchar(window[window_out++]))
    ;
  return 0;
}
