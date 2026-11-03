/* The Twelve Days of Christmas */

char *verse_start = "On the ";

char *ordinals[] = {
    "first", "second", "third",  "fourth", "fifth",    "sixth",
    "seventh", "eighth", "ninth", "tenth", "eleventh", "twelfth",
};

char *verse_middle = " day of Christmas my true love gave to me\n";

/* gifts[n] is the gift given on day n + 1 */
char *gifts[] = {
    "a partridge in a pear tree.\n\n",
    "two turtle doves\nand ",
    "three french hens, ",
    "four calling birds, ",
    "five gold rings;\n",
    "six geese a-laying, ",
    "seven swans a-swimming,\n",
    "eight maids a-milking, ",
    "nine ladies dancing, ",
    "ten lords a-leaping,\n",
    "eleven pipers piping, ",
    "twelve drummers drumming, ",
};

void print_string(char *text) {
  while (*text) {
    putchar(*text++);
  }
}

int main() {
  int day, gift;

  for (day = 1; day <= 12; day++) {
    print_string(verse_start);
    print_string(ordinals[day - 1]);
    print_string(verse_middle);

    /* count the gifts back down to the partridge */
    for (gift = day; gift >= 1; gift--) {
      print_string(gifts[gift - 1]);
    }
  }
  return 0;
}
