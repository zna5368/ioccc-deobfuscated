#include<stdio.h>

/* IOCCC29, w = up, e = down */

/*
 * Quine Pong (and a jumping game).
 *
 * Every run advances the game by one frame and prints the C source code of
 * the next frame.  The printed source is the original compressed program
 * with the current game state baked into its global initialisers and the
 * quine text laid out so that the ball and paddles appear as holes.
 */

#define WIDTH          70
#define HEIGHT         15
#define BALL_WIDTH      3
#define BALL_HEIGHT     2
#define PADDLE_WIDTH    3
#define PADDLE_HEIGHT   5
#define ROBOT_SPEED     2   /* the left paddle's Y is stored doubled */
#define JUMP_VELOCITY  -4

#define LEFT_START_X    5
#define RIGHT_START_X  (WIDTH - PADDLE_WIDTH - LEFT_START_X)   /* 62 */

/* ball_speed_x doubles as the game mode: -1/1 = pong, 2..6 = jump stage */
#define IN_PONG        (ball_speed_x < 2)

/* Characters used inside the quine text in place of a space / newline */
#define QUINE_SPACE    '$'
#define QUINE_NEWLINE  '\''

/* ---- game state (rewritten into the next frame's source) ---- */
int ball_x        = 32;
int ball_y        = 6;
int ball_speed_x  = -1;
int ball_speed_y  = 1;
int left_x        = 5;
int left_y2       = 10;   /* left paddle Y times ROBOT_SPEED */
int right_x       = 62;
int right_y       = 5;
int jump_heights[6] = { 1, 3, 1, 4, 1, 0 };

/*
 * The source text of the generated program.  Whitespace inside these strings
 * is ignored when printing, '$' stands for a space and '\'' for a newline.
 * The odd entries 1..15 hold the state values and are replaced each frame.
 * Entry 17 is empty: it marks where this array itself must be printed.
 */
#define QUINE_LEN 29
char *quine[] = {
    /* 0: header of the generated program */
    "#include<stdio.h>'"
    "#define$z$else'"
    "#define$y$return'"
    "#define$x$int'"
    "#define$w$if('"
    "#define$v$putchar('"
    "#define$B$v$10);'"
    "#define$A$v$92);'"
    "'"
    "/*$IOCCC29,$w$=$up,$e$=$down$*/'"
    "'"
    "x$a=",
    "32",                       /*  1: ball_x       */
    ";x$b=",
    "6",                        /*  3: ball_y       */
    ";x$c=",
    "-1",                       /*  5: ball_speed_x */
    ";x$d=",
    "1",                        /*  7: ball_speed_y */
    ";x$e=",
    "5",                        /*  9: left_x       */
    ";x$f=",
    "10",                       /* 11: left_y2      */
    ";x$g=",
    "62",                       /* 13: right_x      */
    ";x$h=",
    "5",                        /* 15: right_y      */
    ";x$i[6]={1,3,1,4,1,0};char*j[]={",
    "",                         /* 17: the quine array goes here */
    /* 18: print_array_char() */
    "};x$k=0;x$l=1;"
    "x$m(){l++;w$l==1)y!v$44);w$l==2)y!v$34);"
    "char$o=j[k][l-3];w!o){l=0;k++;y!v$34);}"
    "w$o==34){A$y$v$34);}w$o==92){A$y$A}"
    "w$o!=32&&o!=10)y!v$o);y$m();}"
    /* set_quine_value() */
    "void$n(x$o,x$p){asprintf(j+o,\"%i\",p);}"
    /* main(): collision tests */
    "x$main(x$o,char**p){char*q;w$c<2)a+=c;b+=d;"
    "x$r=b+2>f/2&&b<f/2+5;x$s=a+2==g&&b+2>h&&b<h+5;"
    /* pong update */
    "w$c<2){w$a==e+2&&r||s){a-=c;b-=d;c=-c;}"
    "w$a<0||a>67){w$a<0){c=2;d=0;}a=34;b=6;}"
    "w$b<0||b>13){b-=d;d=-d;}"
    "w$f/2>10)f-=2;w$h>10)h--;"
    "w$o>1){w*p[1]==119&&h>0)h--;w*p[1]==101&&h<10)h++;}"
    "s=f/2-b+1;w$s<0)f++;w$s>0)f--;}"
    /* jump update */
    "z{b++;w$d<0)d++;w$b>=13){w$o>1&&*p[1]==119)d=-4;b=13;}"
    "w$f/2<15-i[c-2])f+=2;z$e--;"
    "w$h<15-i[c-1])h++;z$g--;"
    "w$e+3<=0){c++;w$c<7){e=g;f=h*2;g=70;h=15-i[c-1];}"
    "z{e=5;g=62;c=1;d=1;}}"
    "w$a+2==e&&r||s){c=2;e=5;f=28;g=62;h=12;}}"
    /* store the state and print the quine */
    "n(1,a);n(3,b);n(5,c);n(7,d);n(9,e);n(11,f);n(13,g);n(15,h);"
    "for(s=0;s<",
    "29",                       /* 19: QUINE_LEN */
    ";s++){w$s)v$32);q=j[s];r=1;for(char*t=q;*t;t++)w*t==",
    "36",                       /* 21: QUINE_SPACE */
    ")v$32);z$w*t==",
    "39",                       /* 23: QUINE_NEWLINE */
    ")B$z$w*t!=32&&*t!=10){r=0;v*t);w*t==123||*t==125||*t==59)v$32);}"
    "w$r){m();A$B$A$B$"
    "for(o=0;o<15;o++){for(x$u=0;u<70;u++)w$k>=",
    "29",                       /* 25: QUINE_LEN */
    "||u>=a&&o>=b&&u-a<3&&o-b<2"
    "||u>=e&&o>=f/2&&u-e<3&&o-f/2<5"
    "||u>=g&&o>=h&&u-g<3&&o-h<5)v$32);"
    "z$w$m())u++;w$l)A$B}w$l)A$B$for(;k<",
    "29",                       /* 27: QUINE_LEN */
    ";)m();}}B}"
};

/* Cursor into the quine array while printing it as a C initialiser */
int array_index = 0;   /* which string is being printed               */
int array_offset = 1;  /* 0 = comma, 1 = opening quote, 2.. = contents */

/*
 * Print the next visible character of the quine array as C source,
 * including separating commas, quotes and escape backslashes.
 * Spaces and newlines in the data are skipped.
 * Returns 1 when two characters were printed (an escaped character).
 */
int print_array_char(void)
{
    for (;;) {
        array_offset++;

        if (array_offset == 1) {
            putchar(',');
            return 0;
        }
        if (array_offset == 2) {
            putchar('"');
            return 0;
        }

        char ch = quine[array_index][array_offset - 3];

        if (ch == '\0') {
            array_offset = 0;
            array_index++;
            putchar('"');
            return 0;
        }
        if (ch == '"') {
            putchar('\\');
            putchar('"');
            return 1;
        }
        if (ch == '\\') {
            putchar('\\');
            putchar('\\');
            return 1;
        }
        if (ch != ' ' && ch != '\n') {
            putchar(ch);
            return 0;
        }
        /* whitespace: skip it and try the next character */
    }
}

/* Replace a numeric entry of the quine array with a new value */
void set_quine_value(int index, int value)
{
    asprintf(quine + index, "%i", value);
}

static int inside(int col, int row, int x, int y, int width, int height)
{
    return col >= x && row >= y && col - x < width && row - y < height;
}

/*
 * Print the quine array as a picture: the playing field is drawn with the
 * array's characters, leaving blanks where the ball and paddles are.
 */
void print_quine_array(void)
{
    print_array_char();
    putchar('\\');
    putchar('\n');
    putchar('\\');
    putchar('\n');

    for (int row = 0; row < HEIGHT; row++) {
        for (int col = 0; col < WIDTH; col++) {
            if (array_index >= QUINE_LEN
                || inside(col, row, ball_x, ball_y, BALL_WIDTH, BALL_HEIGHT)
                || inside(col, row, left_x, left_y2 / ROBOT_SPEED,
                          PADDLE_WIDTH, PADDLE_HEIGHT)
                || inside(col, row, right_x, right_y,
                          PADDLE_WIDTH, PADDLE_HEIGHT))
                putchar(' ');
            else if (print_array_char())
                col++;
        }
        /* inside a string literal the line has to be continued */
        if (array_offset != 0)
            putchar('\\');
        putchar('\n');
    }
    if (array_offset != 0)
        putchar('\\');
    putchar('\n');

    /* the rest of the array goes after the picture */
    while (array_index < QUINE_LEN)
        print_array_char();
}

/* Print the whole source of the next frame */
void print_quine(void)
{
    for (int item = 0; item < QUINE_LEN; item++) {
        if (item)
            putchar(' ');

        int is_array_slot = 1;
        for (char *t = quine[item]; *t; t++) {
            if (*t == QUINE_SPACE)
                putchar(' ');
            else if (*t == QUINE_NEWLINE)
                putchar('\n');
            else if (*t != ' ' && *t != '\n') {
                is_array_slot = 0;
                putchar(*t);
                /* iocccsize ignores these when followed by whitespace */
                if (*t == '{' || *t == '}' || *t == ';')
                    putchar(' ');
            }
        }
        if (is_array_slot)
            print_quine_array();
    }
    putchar('\n');
}

static void pong_update(int argc, char **argv, int left_hit, int right_hit)
{
    /* bounce off the right edge of the left paddle or the right paddle */
    if (ball_x == left_x + PADDLE_WIDTH - 1 && left_hit || right_hit) {
        ball_x -= ball_speed_x;
        ball_y -= ball_speed_y;
        ball_speed_x = -ball_speed_x;
    }

    /* ball left the field: someone scored */
    if (ball_x < 0 || ball_x > WIDTH - BALL_WIDTH) {
        if (ball_x < 0) {           /* player won: switch to jump game */
            ball_speed_x = 2;
            ball_speed_y = 0;
        }
        ball_x = WIDTH / 2 - BALL_WIDTH / 2;
        ball_y = HEIGHT / 2 - BALL_HEIGHT / 2;
    }

    /* bounce off top and bottom */
    if (ball_y < 0 || ball_y > HEIGHT - BALL_HEIGHT) {
        ball_y -= ball_speed_y;
        ball_speed_y = -ball_speed_y;
    }

    /* keep paddles inside the field */
    if (left_y2 / ROBOT_SPEED > HEIGHT - PADDLE_HEIGHT)
        left_y2 -= ROBOT_SPEED;
    if (right_y > HEIGHT - PADDLE_HEIGHT)
        right_y--;

    /* player input */
    if (argc > 1) {
        if (*argv[1] == 'w' && right_y > 0)
            right_y--;
        if (*argv[1] == 'e' && right_y < HEIGHT - PADDLE_HEIGHT)
            right_y++;
    }

    /* computer follows the ball at half speed */
    int distance = left_y2 / ROBOT_SPEED - ball_y
                   + (PADDLE_HEIGHT / 2 - BALL_HEIGHT / 2);
    if (distance < 0)
        left_y2++;
    if (distance > 0)
        left_y2--;
}

static void jump_update(int argc, char **argv, int left_hit, int right_hit)
{
    int stage = ball_speed_x;

    /* gravity */
    ball_y++;
    if (ball_speed_y < 0)
        ball_speed_y++;

    /* on the ground: maybe jump */
    if (ball_y >= HEIGHT - BALL_HEIGHT) {
        if (argc > 1 && *argv[1] == 'w')
            ball_speed_y = JUMP_VELOCITY;
        ball_y = HEIGHT - BALL_HEIGHT;
    }

    /* obstacles first sink to their height, then slide to the left */
    if (left_y2 / ROBOT_SPEED < HEIGHT - jump_heights[stage - 2])
        left_y2 += ROBOT_SPEED;
    else
        left_x--;

    if (right_y < HEIGHT - jump_heights[stage - 1])
        right_y++;
    else
        right_x--;

    /* left obstacle gone: bring in the next one or return to pong */
    if (left_x + PADDLE_WIDTH <= 0) {
        ball_speed_x++;
        if (ball_speed_x < 7) {
            left_x = right_x;
            left_y2 = right_y * ROBOT_SPEED;
            right_x = WIDTH;
            right_y = HEIGHT - jump_heights[ball_speed_x - 1];
        } else {
            left_x = LEFT_START_X;
            right_x = RIGHT_START_X;
            ball_speed_x = 1;
            ball_speed_y = 1;
        }
    }

    /* hit an obstacle: restart the jump game */
    if (ball_x + BALL_WIDTH - 1 == left_x && left_hit || right_hit) {
        ball_speed_x = 2;
        left_x = LEFT_START_X;
        left_y2 = ROBOT_SPEED * (HEIGHT - jump_heights[0]);
        right_x = RIGHT_START_X;
        right_y = HEIGHT - jump_heights[1];
    }
}

int main(int argc, char **argv)
{
    if (IN_PONG)
        ball_x += ball_speed_x;
    ball_y += ball_speed_y;

    int left_y = left_y2 / ROBOT_SPEED;
    int left_hit = ball_y + BALL_HEIGHT > left_y
                   && ball_y < left_y + PADDLE_HEIGHT;
    int right_hit = ball_x + BALL_WIDTH - 1 == right_x
                    && ball_y + BALL_HEIGHT > right_y
                    && ball_y < right_y + PADDLE_HEIGHT;

    if (IN_PONG)
        pong_update(argc, argv, left_hit, right_hit);
    else
        jump_update(argc, argv, left_hit, right_hit);

    set_quine_value(1, ball_x);
    set_quine_value(3, ball_y);
    set_quine_value(5, ball_speed_x);
    set_quine_value(7, ball_speed_y);
    set_quine_value(9, left_x);
    set_quine_value(11, left_y2);
    set_quine_value(13, right_x);
    set_quine_value(15, right_y);

    print_quine();
}
