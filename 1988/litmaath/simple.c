/*
 * Sort the characters of each command line argument and print each
 * sorted argument on its own line.
 */
int main(int argc, char **argv)
{
	int arg, i, j;
	char *word, smallest;

	for (arg = 1; arg < argc; arg++) {
		word = argv[arg];

		/* selection sort: move the smallest remaining character to the front */
		for (i = 0; word[i] != '\0'; i++) {
			for (j = i + 1; word[j] != '\0'; j++) {
				if (word[i] > word[j]) {
					smallest = word[j];
					word[j] = word[i];
					word[i] = smallest;
				}
			}
			putchar(word[i]);
		}
		putchar('\n');
	}
	return 0;
}
