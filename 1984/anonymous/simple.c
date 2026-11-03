int position;

/*
 * Write a single character to standard output.
 */
put_char(fd, character, count) char *character;
{ write(fd, character, count); }

main() {
  char *message = "hello, world!\n";

  for (position = 0; message[position] != '\0'; position++)
    put_char(1, &message[position], 1);
}
