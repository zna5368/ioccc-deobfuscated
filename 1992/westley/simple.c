/*
 * Print an 80-column world map made of '!' (land) and ' ' (sea), marking the
 * given latitude/longitude with '"' (on land) or '#' (on sea).
 */

#define MAP_WIDTH 80

/*
 * Run-length encoded map. Each character is a run length: 'A' = 1, 'B' = 2,
 * ... and any character <= '@' (64) is a run of 0. Runs alternate between
 * land ('!') and sea (' '), starting with land. The trailing
 * " H E L L O,    W O R L D! " is just a decorative way of encoding the
 * final rows of sea: the letters are runs of sea and the spaces and
 * punctuation are empty land runs.
 */
static char map_runs[] = "@NKACLCCGZAAQBEAADAFaISADJABBA^"
                         "SNLGAQABDAXIMBAACTBATAHDBAN"
                         "ZcEMMCCCCAAhEIJFAEAAABAfHJE"
                         "TBdFLDAANEfDNBPHdBcBBBEA_AL"
                         " H E L L O,    W O R L D! ";

main(argc, argv) char **argv;
{
  int run;
  int run_index;
  int column;
  int position;
  int marker;
  int is_sea;
  int ch;

  if (argc < 3)
    return;

  /*
   * One map row is 80 characters and covers 10 degrees of latitude; one
   * character covers 5 degrees of longitude. 596 is the position of
   * latitude 0, longitude 0 (row 7, column 36).
   */
  marker = 596 - atoi(argv[1]) / 10 * MAP_WIDTH + atoi(argv[2]) / 5;

  position = 0;
  column = 0;
  /* the original indexes the map with argc - 3, so extra arguments skip runs */
  for (run_index = argc - 3; map_runs[run_index] != '\0'; run_index++) {
    is_sea = run_index & 1;
    for (run = map_runs[run_index] - '@'; run > 0; run--) {
      if (position == marker)
        ch = is_sea ? '#' : '"';
      else
        ch = is_sea ? ' ' : '!';
      putchar(ch);
      position++;

      if (++column % MAP_WIDTH == 0)
        putchar('\n');
    }
  }
}
