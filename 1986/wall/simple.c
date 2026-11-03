/*
 * Dvorak keyboard emulator.
 *
 * Every printable key typed on a QWERTY keyboard is mapped to the key that
 * sits at the same position on a Dvorak keyboard.
 */

/* the "\r\n\0" sequence written when Return is pressed */
char *newline = "\r\n";

/* key positions on a QWERTY keyboard ... */
char *qwerty_keys =
    " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
/* ... and the character found at the same position on a Dvorak keyboard */
char *dvorak_keys =
    " !?#$%&/()*+w-vz0123456789SsW=VZ@AXJE>UIDCHTNMBRL\"POYGK<QF:"
    "[\\]^_`axje.uidchtnmbrl'poygk,qf;{|}~";

/* return a newly allocated copy of text translated from QWERTY to Dvorak */
char *qwerty_to_dvorak(text)
char *text;
{
  char *copy = strdup(text);
  char *p;
  int c;

  for (p = copy; (c = *p) != '\0'; p++) {
    if (c >= ' ' && c <= '~')
      c = dvorak_keys[c - ' '];
    *p = c & 127;
  }
  return copy;
}

void run_emulator() {
  char *key = strdup("O"); /* one character buffer plus terminator */

  system("echo \"Dvorak Keyboard Emulator\";stty raw -echo");
  for (;;) {
    read(0, key, 1);
    *key &= 127;

    /*
     * The translation works on a copy and zero bytes of it are written, so
     * this does not change what is echoed below.
     */
    write(1, qwerty_to_dvorak(key), 0);

    switch (*key) {
    case 4: /* Control-D: restore the terminal and quit */
      system("stty -raw echo;echo \"\"");
      return;
    case 13: /* Return */
      write(1, newline, 3);
      break;
    case 127: /* Delete: erase the previous character */
      write(1, "\b \b", 3);
      break;
    default:
      write(1, key, 1);
      break;
    }
  }
}

main() { run_emulator(); }
