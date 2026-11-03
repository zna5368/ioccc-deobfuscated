/*
 * Prime number sieve built from lazy, infinite streams.
 *
 * A stream is a cell holding one value plus a "generator" (a closure)
 * that produces the next cell on demand.  The sieve of Eratosthenes is
 * built by stacking a filter generator on top of the integer stream for
 * every prime found.  Each prime is printed in binary on its own line.
 */

struct generator;

/* one element of a lazy stream: a value and how to compute the rest */
struct cell {
  int value;
  struct generator *rest;
};

/* a closure: function to call, an integer argument and a source stream */
struct generator {
  struct cell *(*next)(int arg, struct generator *source);
  int arg;
  struct generator *source;
};

/* evaluate a generator, producing the next cell of its stream */
static struct cell *force(struct generator *gen) {
  return gen->next(gen->arg, gen->source);
}

/* the stream of integers n, n+1, n+2, ... */
struct cell *count_from(int n, struct generator *unused) {
  struct cell *cell = malloc(sizeof(struct cell));
  cell->value = n;
  cell->rest = malloc(sizeof(struct generator));
  cell->rest->next = count_from;
  cell->rest->arg = n + 1;
  return cell;
}

/* the stream of values from source that are not multiples of prime */
struct cell *remove_multiples(int prime, struct generator *source) {
  struct generator *self = source; /* recycled as the closure for the rest */
  struct cell *cell;

  do {
    cell = force(source);
    source = cell->rest;
  } while (cell->value % prime == 0);

  self->next = remove_multiples;
  self->arg = prime;
  self->source = source;
  cell->rest = self;
  return cell;
}

/*
 * The first value of the source is prime.  Return it, and make the rest
 * of the stream the sieve of the source with that prime's multiples removed.
 */
struct cell *sieve(int unused, struct generator *source) {
  struct cell *cell = force(source);
  struct generator *remaining = cell->rest;
  struct generator *filter;

  /* reuse the source closure as the sieve closure for the rest */
  cell->rest = source;
  source->next = sieve;

  filter = malloc(sizeof(struct generator));
  filter->next = remove_multiples;
  filter->arg = cell->value;
  filter->source = remaining;
  source->source = filter;
  return cell;
}

/* print a newline then n in binary, most significant bit first */
void print_binary(int n) {
  if (n == 0) {
    putchar('\n');
    return;
  }
  print_binary(n / 2);
  putchar(n % 2 + '0');
}

/* print every value of the stream forever */
struct cell *print_stream(int unused, struct generator *stream) {
  struct cell *cell;

  for (;;) {
    cell = force(stream);
    print_binary(cell->value);
    stream = cell->rest;
  }
}

main(argc, argv) char **argv;
{
  struct generator *primes;
  struct generator *integers;

  integers = malloc(sizeof(struct generator));
  integers->next = count_from;
  integers->arg = 2;

  primes = malloc(sizeof(struct generator));
  primes->next = sieve;
  primes->source = integers;

  print_stream(0, primes);
}
