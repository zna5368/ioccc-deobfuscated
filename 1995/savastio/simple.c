#include <stdio.h>

#define TERMINATOR 0xFFFF   /* marks the end of the number's digits */
#define BLOCK_SIZE 1000     /* 4-digit buckets per block */
#define BUCKET_BASE 10000   /* each bucket holds 4 decimal digits */
#define GROUPS_PER_LINE 19  /* 4-digit groups printed per line */

/*
 * A big number is a doubly linked list of blocks. Each block holds
 * BLOCK_SIZE buckets of 4 decimal digits, least significant bucket first.
 * The bucket after the most significant one holds TERMINATOR.
 */
struct block {
  struct block *next, *prev;
  unsigned short bucket[BLOCK_SIZE];
};

struct block *malloc();

static struct block *new_block(void) {
  struct block *b = malloc(sizeof(struct block));
  b->next = 0;
  b->prev = 0;
  return b;
}

/* Move to the next block of the output number, creating it if needed. */
static struct block *next_output_block(struct block *out) {
  if (!out->next) {
    out->next = new_block();
    out->next->prev = out;
  }
  return out->next;
}

main() {
  struct block *result, *scratch, *out, *in;
  unsigned long carry;
  long multiplier, count, n;
  unsigned short *src, *dst;
  static const char prompt[] = "Enter number: ";

  for (n = 0; n < 14; ++n)
    putchar(prompt[n]);
  /* n is a long but read with %d; on bad input n stays 14 */
  scanf("%d", &n);

  /* result = 1 */
  result = new_block();
  scratch = new_block();
  out = result;
  result->bucket[0] = 1;
  result->bucket[1] = TERMINATOR;

  for (multiplier = 2; multiplier <= n; ++multiplier) {
    /* scratch = result * multiplier, then swap the two buffers */
    out = scratch;
    in = scratch = result;
    src = in->bucket;
    result = out;
    dst = out->bucket;
    count = carry = 0;

    while (in->next || *src != TERMINATOR) {
      carry += multiplier * *src++;
      *dst++ = carry % BUCKET_BASE;
      carry /= BUCKET_BASE;
      if (++count > BLOCK_SIZE) {
        out = next_output_block(out);
        dst = out->bucket;
        count = 0;
        in = in->next;
        src = in->bucket;
      }
    }
    /* spill the remaining carry into new buckets */
    while (carry) {
      *dst++ = carry % BUCKET_BASE;
      carry /= BUCKET_BASE;
      if (++count >= BLOCK_SIZE) {
        out = next_output_block(out);
        dst = out->bucket;
        count = 0;
      }
    }
    *dst = TERMINATOR;
  }

  /* find the most significant bucket in the last block */
  count = 0;
  for (n = 0; n < BLOCK_SIZE && out->bucket[n] != TERMINATOR; ++n)
    ;

  /* print buckets from most to least significant, walking blocks backwards */
  for (; out; out = out->prev, n = BLOCK_SIZE) {
    for (--n; n >= 0; --n, ++count) {
      if (count == 0)
        printf("%4d", out->bucket[n]);
      else if (count % GROUPS_PER_LINE)
        printf("%04d", out->bucket[n]);
      else
        printf("\n%04d", out->bucket[n]);
    }
  }
  putchar('\n');
}
