/*
 * Prints "unix" by building it from two pieces:
 *
 *   format: "\021%six\012\0" with its first byte skipped  -> "%six\n"
 *   suffix: "fun" advanced by ('a' - 0x60) == 1 character -> "un"
 *
 * so printf("%six\n", "un") writes "unix\n".
 */
main()
{
	char *format = "%six\n";
	char *suffix = "un";

	printf(format, suffix);
}
