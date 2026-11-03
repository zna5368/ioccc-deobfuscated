/** J.K.Bonfield **
^	<						 	 							
^	>						 	 							
<			#				>						v		
<			 				 			<					
<			>				>			<		v		
X			 				 			X				j
x			 				 			X				j
X	 						x	^						k
^	 						 	^							
^	#						 									
X			 				X			X				X

^	v						 	 							
v						 	 						v		
v						#	 									
v						X	 						 		
v						x	 						 		
>				#			<						v		
>				 			 				>				
X				 			 				X			l
x				 			 				X			l

41 20
########################################
#                                      #
#  <   <   <   <   <>  <   <   <       #
#       >   >   >  <>   >   >   >   >  #
#  <   <   <   <   <>  <   <   <       #
#       >   >   >  <>   >   >   >   >  #
#  <   <   <   <   <>  <   <   <       #
#       >   >   >  <>   >   >   >   >  #
#                                      #
#                                      #
#                                      #
#                                      #
#                                      #
#                                      #
#                                      #
#                                      #
#                                      #
#                                      #
#           X                          #
########################################*/
#include <stdio.h>
#include <curses.h>
#include <stdlib.h>
#include <setjmp.h>
#include <sys/time.h>

/*
 * CABBAGE: a Cellular Automata Based, Beautifully Abysmal Game Environment.
 *
 * With no argument the game description is read from this very source file
 * (the comment above is the Space Invaders game).  Wildcards in that file are
 * TAB characters; in any other game file they are '*'.
 *
 * Game file format:
 *
 *   <forward rules>\n
 *   \n
 *   <backward rules>\n
 *   \n
 *   <width> <height>\n
 *   <map: height lines of width bytes, each including the newline>
 *
 * Every rule is one line of 19 characters:
 *
 *   e abcdfghi 5 12346789 K
 *
 * 'e' is the cell that must be in the center, "abcdfghi" are the eight
 * neighbours to match (row by row, skipping the center), '5' is the new
 * center, "12346789" the new neighbours and 'K' the key that must have been
 * pressed for the rule to be active.  A wildcard on the match side matches
 * anything, on the replace side it keeps the old cell, and as the key it means
 * the rule is always active.
 */

/* left over from old curses implementations that needed these defined */
int _tty_ch;
bool _echoit;
bool _rawmode;
int _tty;

#define FRAME_USEC (1 << 17) /* ~0.13 seconds between generations */

/* neighbour order used by the rules: a b c / d . f / g h i */
#define NEIGHBOURS 8
static const int neighbour_dy[NEIGHBOURS] = {-1, -1, -1, 0, 0, 1, 1, 1};
static const int neighbour_dx[NEIGHBOURS] = {-1, 0, 1, -1, 1, -1, 0, 1};

/* layout of the 18 characters following the center character of a rule */
#define MATCH 0    /* 8 neighbour cells to match */
#define NEW_CENTER 8 /* replacement for the center cell */
#define REPLACE 9  /* 8 neighbour replacement cells */
#define KEY 17     /* key that activates the rule */

typedef struct rule {
    char pattern[20];  /* rule text after the center character */
    struct rule *next; /* next rule with the same center character */
} rule;

/*
 * rules[0] are applied scanning top-left to bottom-right,
 * rules[1] are applied scanning bottom-right to top-left.
 * Each pass is indexed by the character in the center of the rule.
 */
rule *rules[2][256];

struct timeval frame_delay = {0, FRAME_USEC};

char **map;     /* map[row][column] */
char key;       /* key pressed this generation, or 0 */
char wildcard;  /* matches anything / keeps cell unchanged */

/* apply every rule for the cell at (row, col) from the given rule list */
static void apply_rules(rule *list, int row, int col)
{
    rule *r;
    int n;

    for (r = list; r; r = r->next) {
        char *p = r->pattern;

        if (p[KEY] != wildcard && p[KEY] != key)
            continue;

        /* all eight neighbours must match */
        for (n = 0; n < NEIGHBOURS; n++) {
            char want = p[MATCH + n];
            if (want != wildcard &&
                want != map[row + neighbour_dy[n]][col + neighbour_dx[n]])
                break;
        }
        if (n < NEIGHBOURS)
            continue;

        /* rewrite the center and then the neighbours */
        if (p[NEW_CENTER] != wildcard)
            map[row][col] = p[NEW_CENTER];
        for (n = 0; n < NEIGHBOURS; n++)
            if (p[REPLACE + n] != wildcard)
                map[row + neighbour_dy[n]][col + neighbour_dx[n]] =
                    p[REPLACE + n];
    }
}

/* run the game forever; width includes the newline at the end of each row */
static void play(int width, int height)
{
    clear(), noecho(), cbreak();

    for (;;) {
        int row, col;
        int start_row;
        fd_set input;

        /* draw the map (without the newlines) */
        for (row = 0; row < height; row++)
            for (col = 0; col < width - 1; col++)
                mvaddch(row, col, map[row][col]);
        refresh();

        /* wait one frame for a key press */
        FD_ZERO(&input);
        FD_SET(0, &input);
        frame_delay.tv_usec = FRAME_USEC;
        key = select(1, &input, 0, 0, &frame_delay) ? getch() : 0;

        /*
         * The forward scan starts on row 1 when stdin is readable, but on
         * row 0 (the top border) otherwise.  The original program reused
         * the row counter as the select() descriptor set.
         */
        start_row = FD_ISSET(0, &input) ? 1 : 0;

        /* forward pass: top-left to bottom-right */
        for (row = start_row; row < height - 1; row++)
            for (col = row == start_row ? 0 : 1; col < width; col++)
                apply_rules(rules[0][map[row][col]], row, col);

        /* backward pass: bottom-right to top-left */
        for (row = height - 2; row > 0; row--)
            for (col = width - 1; col > 0; col--)
                apply_rules(rules[1][map[row][col]], row, col);
    }
}

int main(int argc, char **argv)
{
    FILE *game = fopen(argc != 2 ? __FILE__ : argv[1], "r");
    int pass = 0;
    int width, height;
    int row;

    if (!game)
        return 1;
    wildcard = argc != 2 ? '\t' : '*';

    for (;;) {
        char c = getc(game);
        rule *r;

        if (c == '\n') {
            if (pass++ == 0)
                continue; /* end of forward rules, start backward rules */

            /* end of backward rules: read the map and play */
            fscanf(game, "%d %d\n", &width, &height);
            map = (char **)malloc(sizeof(char *) * height * (width + 1));
            for (row = 0; row < height; row++)
                map[row] = (char *)&map[height] + row * width;
            initscr();
            fread(*map, height * width, 1, game);
            play(width, height);
        }

        /* add a rule to the front of the list for its center character */
        r = (rule *)malloc(sizeof(rule));
        r->next = rules[pass][c];
        rules[pass][c] = r;
        fgets(r->pattern, 20, game);
    }
    endwin();
}
