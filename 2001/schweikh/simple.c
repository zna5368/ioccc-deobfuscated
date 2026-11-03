/*
 * Glob pattern matcher: '*' matches zero or more characters and '?' matches
 * exactly one character. The pattern must match the whole string.
 *
 * Usage: ./simple string pattern
 * Exit status is 0 if the pattern matches, 1 otherwise.
 */

int glob_match(char *string, char *pattern);

int main(int argc, char **argv) {
  if (argc < 3) {
    return 1;
  }
  return !glob_match(argv[1], argv[2]);
}

/* Returns nonzero when pattern matches the whole of string. */
int glob_match(char *string, char *pattern) {
  if (*pattern == '*') {
    /* Let '*' match nothing: skip it and try the rest of the pattern. */
    if (glob_match(string, pattern + 1)) {
      return 1;
    }
    /* Otherwise let '*' swallow one more character and try again. */
    if (*string != '\0' && glob_match(string + 1, pattern)) {
      return 1;
    }
    return 0;
  }

  if (*string == '\0') {
    /* End of string: matches only if the pattern is also finished. */
    return *pattern == '\0';
  }

  /* '?' matches any single character; anything else must match exactly. */
  if (*pattern == '?' || *pattern == *string) {
    return glob_match(string + 1, pattern + 1);
  }
  return 0;
}
