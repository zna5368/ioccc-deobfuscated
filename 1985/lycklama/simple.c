/*
 * The original has no #include lines; write(), read() and usleep() are
 * implicitly declared.  Z is the delay in microseconds given with -DZ=...
 * (a delay of 0 disables sleeping).
 */
#define STDIN 0
#define STDOUT 1
#define CHUNK_SIZE 80 /* (1 << 6) + (1 << 4) */

/* write to stdout, pausing a little before every write so the effect is visible */
#define put(text, length) (void)write(STDOUT, text, (unsigned)(length), Z && usleep(Z))

main() {
  char bytes_read = 0, visible, steps_left, chunk[CHUNK_SIZE];

  /* read the input 80 bytes at a time until end of file */
  while ((bytes_read = read(STDIN, chunk, CHUNK_SIZE))) {
    /* slide the chunk (minus its last byte) to the right, one column per step */
    for (steps_left = CHUNK_SIZE - 1; steps_left; steps_left--) {
      /* print as much of the chunk as still fits on the line */
      visible = bytes_read - 1 < steps_left ? bytes_read - 1 : steps_left;
      put(chunk, visible);

      /* move the cursor back to where the text started */
      for (; visible; visible--)
        put("\b", 1);

      /* blank the first column and step one column to the right */
      put(" ", 1);
    }
    put("\n", 1);
  }
}
