/*
 * Conway's Game of Life drawn in the X11 root window background.
 *
 * The program speaks the X11 wire protocol directly over a socket (no Xlib).
 * Like the original it includes no headers; the Makefile supplies
 * sys/types.h, sys/socket.h and unistd.h with -include.
 */

#define AF_UNIX_FAMILY 1
#define AF_INET_FAMILY 2

/* X11 request opcodes used */
#define X_ChangeWindowAttributes 2
#define X_CreatePixmap 53
#define X_CreateGC 55
#define X_ClearArea 61
#define X_CopyPlane 63
#define X_PutImage 72

#define GCForeground 4
#define GCBackground 8
#define CWBackPixmap 1

/* the life board is computed in horizontal bands of this many rows */
#define BAND_ROWS 30
/* resource ids are spaced this far apart from the server's id base */
#define ID_STEP 8192

/* one board row is words_per_row 32-bit words, one bit per pixel */
unsigned words_per_row, total_words, rows;

/*
 * Single buffer shared by the socket address, the connection setup, the
 * server's reply and every request. Some request pad bytes are skipped
 * rather than written, so they keep whatever was in the buffer before.
 */
unsigned buffer[9999];
char *pos;

static void put8(int value) { *pos++ = value; }
static void put16(int value) {
  *(short *)pos = value;
  pos += 2;
}
static void put32(unsigned value) {
  *(unsigned *)pos = value;
  pos += 4;
}
static int get8(void) { return *pos++; }
static int get16(void) {
  int v = *(short *)pos;
  pos += 2;
  return v;
}
static unsigned get32(void) {
  unsigned v = *(unsigned *)pos;
  pos += 4;
  return v;
}
static void skip(int bytes) { pos += bytes; }

/* send the request built in the buffer and rewind to the buffer start */
static void send_request(int conn, int length_in_words) {
  write(conn, pos = (char *)buffer, 4 * length_in_words);
}

/* read a decimal number into the next byte; the terminating character
 * (anything without bit 0x10 set, such as '.' or '\0') is consumed.
 * Note ':' (0x3a) has that bit, so "1.2.3.4:0.0" is not handled. */
static void parse_octet(char **text) {
  int ch;
  while ((ch = *(*text)++) & 16)
    *pos = 10 * *pos + ch - '0';
  pos++;
}

/* fill the board with pseudo-random bits seeded by the process id */
static void randomize(unsigned *cells) {
  unsigned *end = cells + total_words;
  unsigned seed = getpid();
  while (cells < end)
    *cells++ = seed = (3 * seed) ^ (seed / 2);
}

/* reverse the order of all words of the board */
static void reverse_words(unsigned *cells) {
  unsigned *low = cells, *high = cells + total_words, tmp;
  while (high > low) {
    tmp = *--high;
    *high = *low;
    *low++ = tmp;
  }
}

/* reverse the order of all bytes of the board */
static void reverse_bytes(unsigned *cells) {
  char *low = (char *)cells, *high = low + total_words * 4, tmp;
  while (high > low) {
    tmp = *--high;
    *high = *low;
    *low++ = tmp;
  }
}

/*
 * Shift a word one pixel to the right (pixels are stored most significant
 * bit first). The bit coming in at the top is the lowest bit of the word to
 * the left, which was saved 32 calls earlier in the history register: the
 * caller walks down a column of 32 rows, so 32 calls ago was the same row
 * one word to the left.
 */
static unsigned shift_in(unsigned word, unsigned *history) {
  unsigned shifted = (*history & 0x80000000u) | word >> 1;
  *history = *history << 1 | (word & 1);
  return shifted;
}

/* three horizontally adjacent cells of one row, 32 positions at a time */
struct row_cells {
  unsigned center;     /* the cell itself */
  unsigned pair_sum;   /* left + right, low bit  (left ^ right) */
  unsigned pair_carry; /* left + right, high bit (left & right) */
};

static void load_row(unsigned word, unsigned *history1, unsigned *history2,
                     struct row_cells *row) {
  unsigned right = word;
  unsigned center = shift_in(right, history1);
  unsigned left = shift_in(center, history2);
  row->center = center;
  row->pair_sum = left ^ right;
  row->pair_carry = left & right;
}

/* neighbour count as bit planes: ones + 2*twos, many = "4 or more" */
struct counter {
  unsigned ones, twos, many;
};

static void add_pair(struct counter *n, struct row_cells *row) {
  unsigned carry = n->ones & row->pair_sum;
  n->many |= carry & n->twos;
  n->twos ^= carry;
  n->ones ^= row->pair_sum;
  n->many |= row->pair_carry & n->twos;
  n->twos ^= row->pair_carry;
}

/*
 * Compute the next generation of `cells` into `next`, then copy it back into
 * `cells`. The board wraps around at the edges. Each output bit is the cell
 * one pixel left of its bit position, which the final shift corrects.
 */
static void next_generation(unsigned *cells, unsigned *next) {
  unsigned width = words_per_row;
  unsigned *src, *dst, *band_top, *last, *in, *out;
  unsigned history1 = 0, history2 = 0, band, col, row, k, carry, word;
  unsigned above_center, above_sum, above_carry;
  struct row_cells line;
  struct counter n;

  /* the padding rows before and after the board copy the opposite edge */
  src = cells + total_words - width;
  dst = cells - width;
  for (k = 0; k < width; k++) {
    dst[k] = src[k];
    src[k + width] = cells[k];
  }

  for (band = 0; band < rows; band += BAND_ROWS) {
    band_top = cells + band * width - width; /* the row above the band */
    last = band_top - 1 + (BAND_ROWS + 2) * width;

    /* prime the shift registers with the last word of each of the 32 rows,
     * which is the left neighbour of column 0 (wrapping around) */
    for (in = band_top - 1 + width; in <= last; in += width)
      shift_in(shift_in(*in, &history1), &history2);

    for (col = 0; col < width; col++) {
      in = band_top + col;
      out = next + band * width + col;

      /* row above the first output row: full 3-cell sum */
      load_row(*in, &history1, &history2, &line);
      in += width;
      n.twos = line.pair_carry | (line.pair_sum & line.center);
      n.ones = line.pair_sum ^ line.center;
      load_row(*in, &history1, &history2, &line);
      in += width;

      for (row = 0; row < BAND_ROWS; row++) {
        /* add the left and right neighbours of the current row */
        n.many = 0;
        add_pair(&n, &line);
        above_center = line.center;
        above_sum = line.pair_sum;
        above_carry = line.pair_carry;

        /* add all three cells of the row below */
        load_row(*in, &history1, &history2, &line);
        in += width;
        add_pair(&n, &line);
        n.twos ^= n.ones & line.center;

        /* alive if the count is 3, or 2 and the cell was alive */
        *out = n.twos & ~n.many & ((n.ones ^ line.center) | above_center);
        out += width;

        /* the current row's 3-cell sum is "above" for the next row */
        n.twos = above_carry | (above_sum & above_center);
        n.ones = above_sum ^ above_center;
      }
    }
  }

  /* shift every row one pixel left (with wrap-around) back into cells */
  for (row = 0; row < rows; row++) {
    src = next + row * width;
    dst = cells + row * width;
    carry = src[0] >> 31;
    for (k = width; k-- > 0;) {
      word = src[k];
      dst[k] = (word << 1) + carry;
      carry = word >> 31;
    }
  }
}

int main(int argc, char **argv) {
  int conn, attempt = 0, family = argc;
  int client_big_endian, swap_bytes, reverse_order, vendor_length;
  unsigned id_base, root, white_pixel, black_pixel, depth, max_rows;
  unsigned bitmap, bitmap_gc, background, formats, y, chunk;
  unsigned *cells, *next;
  char *setup, *text;

  pos = (char *)buffer;

  /* socket address */
  if (argc > 1) {
    put16(AF_INET_FAMILY);
    put8(6000 >> 8); /* port 6000, network byte order */
    put8(6000 & 255);
    text = argv[1];
    parse_octet(&text);
    parse_octet(&text);
    parse_octet(&text);
    parse_octet(&text);
  } else {
    put16(AF_UNIX_FAMILY);
    text = "/tmp/.X11-unix/X0";
    while (*text)
      put8(*text++);
    put8(0);
  }

  /* connection setup request: byte order 'l' or 'B', protocol 11.0,
   * no authorization */
  setup = pos;
  put16('B' << 8 | 'l');
  put16(11);
  put32(0);
  put32(0);
  client_big_endian = *setup == 'B';

  /* try socket types 3, 2, 1, ... until a connection works */
  while (connect(conn = socket(family, 3 - attempt++, 0), (void *)buffer,
                 24 - family * 4) ||
         write(conn, setup, 12) != 12 || read(conn, pos, 8) != 8)
    ;

  /* reply header: status, unused, major, minor, length of the rest */
  skip(4);
  skip(2);
  read(conn, buffer, 4 * get16());
  pos = (char *)buffer;

  skip(4); /* release number */
  id_base = get32();
  skip(4); /* resource id mask */
  skip(4); /* motion buffer size */
  vendor_length = get16();
  max_rows = get16() - 6;
  skip(1);                                 /* number of screens */
  formats = get8();                        /* number of pixmap formats */
  swap_bytes = client_big_endian ^ get8(); /* image byte order */
  reverse_order = !get8();                 /* bitmap bit order LSB first */
  skip((11 + vendor_length) / 4 * 4 + 8 * formats);

  /* first screen */
  root = get32();
  skip(4); /* default colormap */
  white_pixel = get32();
  black_pixel = get32();
  skip(4); /* current input masks */
  words_per_row = get16() / 32;
  rows = get16() / BAND_ROWS * BAND_ROWS;
  skip(14);
  depth = get8();
  max_rows /= words_per_row;
  pos = (char *)buffer;

  /* CreateGC id_base on root: foreground black, background white */
  put8(X_CreateGC);
  skip(1);
  put16(6);
  put32(id_base);
  put32(root);
  put32(GCForeground | GCBackground);
  put32(black_pixel);
  put32(white_pixel);
  send_request(conn, 6);

  /* CreatePixmap of depth 1 to hold the board */
  bitmap = id_base + ID_STEP;
  put8(X_CreatePixmap);
  put8(1);
  put16(4);
  put32(bitmap);
  put32(root);
  put16(words_per_row * 32);
  put16(rows);
  send_request(conn, 4);

  /* CreateGC for drawing into the bitmap */
  bitmap_gc = bitmap + ID_STEP;
  put8(X_CreateGC);
  skip(1);
  put16(4);
  put32(bitmap_gc);
  put32(bitmap);
  put32(0);
  send_request(conn, 4);

  /* CreatePixmap of the screen depth for the root background */
  background = bitmap_gc + ID_STEP;
  put8(X_CreatePixmap);
  put8(depth);
  put16(4);
  put32(background);
  put32(root);
  put16(words_per_row * 32);
  put16(rows);
  send_request(conn, 4);

  /* ChangeWindowAttributes: use it as the root window background */
  put8(X_ChangeWindowAttributes);
  skip(1);
  put16(4);
  put32(root);
  put32(CWBackPixmap);
  put32(background);
  send_request(conn, 4);

  /* two boards, each with a spare row before and after for wrap-around */
  total_words = rows * words_per_row;
  cells = (unsigned *)sbrk(total_words * 4 + words_per_row * 8) + words_per_row;
  next = (unsigned *)sbrk(total_words * 4 + words_per_row * 8) + words_per_row;
  randomize(cells);

  while (1) {
    /* convert the board to the server's byte and bit order */
    if (swap_bytes ^ reverse_order)
      reverse_words(cells);
    if (swap_bytes)
      reverse_bytes(cells);

    /* PutImage the board into the bitmap, as many rows as fit per request */
    for (y = 0; y < rows; y += chunk) {
      put8(X_PutImage);
      put8(0); /* format Bitmap */
      chunk = rows - y;
      if (chunk > max_rows)
        chunk = max_rows;
      put16(6 + words_per_row * chunk);
      put32(bitmap);
      put32(bitmap_gc);
      put16(words_per_row * 32);
      put16(chunk);
      put16(0);
      put16(y);
      put8(0); /* left pad */
      put8(1); /* depth */
      send_request(conn, 6);
      write(conn, cells + y * words_per_row, chunk * words_per_row * 4);
    }

    /* CopyPlane the bitmap into the background pixmap in black and white */
    put8(X_CopyPlane);
    skip(1);
    put16(8);
    put32(bitmap);
    put32(background);
    put32(id_base);
    put32(0); /* source x, y */
    put32(0); /* destination x, y */
    put16(words_per_row * 32);
    put16(rows);
    put32(1); /* bit plane */
    send_request(conn, 8);

    /* ClearArea of the whole root window so the new background shows */
    put8(X_ClearArea);
    put8(0);
    put16(4);
    put32(root);
    put32(0);
    put32(0);
    send_request(conn, 4);

    /* back to the native order */
    if (swap_bytes)
      reverse_bytes(cells);
    if (swap_bytes ^ reverse_order)
      reverse_words(cells);

    next_generation(cells, next);
  }
}
