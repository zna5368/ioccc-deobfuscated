#include <stdio.h>

/*
 * Sinon: a timing game played by recompiling.
 *
 * Every compile-and-run of this program is one shot. The program prints a new
 * C source file that holds the updated game state, so the state lives in the
 * source code. The output is the original obfuscated game, which is why that
 * text is stored in NEXT_GENERATION_CODE below.
 *
 * The inputs are the clock time of the compilation (__TIME__) and the
 * optimization level, which selects the weapon.
 */

#define SECONDS_PER_DAY 86400
#define TIME_LIMIT 60

#define GRID_ROWS 18
#define GRID_COLUMNS 11
#define ENEMY_COUNT (GRID_ROWS * GRID_COLUMNS)
#define FACE_COUNT 4

#define AMMO_BAR_LENGTH 25
#define CLOSING_PADDING 77

#define OUTPUT_BUFFER_SIZE 9999

enum weapon { HECATE, GLOCK, WEAPON_COUNT };

static const int weapon_damage[WEAPON_COUNT] = { 17, 11 };
static const int weapon_cooldown[WEAPON_COUNT] = { 5, 3 };

/* Game state. The printed source file carries these values to the next run. */
int start_time = 0;
int last_shot_time[WEAPON_COUNT] = { 0, 0 };
int shots_fired[WEAPON_COUNT] = { 0, 0 };

char output[OUTPUT_BUFFER_SIZE];
char *output_end = output;

const char *enemy_faces[FACE_COUNT] = { "('A`)", "(A`)", "('A)", "('A`)" };

/* Source code of the next generation, exactly as the original prints it. */
const char NEXT_GENERATION_CODE[] =
    "*s=__TIME__;extern void w(){G(g,*o)for(;"
    "g-- ;);}extern void x(){}extern void y(){"
    "}static void z (char*p){for(;(*d=*p++)!=0;"
    "d++)*d-=061*(59==*d||*d==81);}int main(){"
    "char N[]=\"<;iQ=/*do<;dQ=ook8?boc;aQ=odr8>oc;_Q<ox8>oc\" \";^Q<d|8"
    "=o;]Q<dj8<Pk8<c]'?;QQdi8?P'Q)d8<9`8?c;Qdh8=P']Q<`c8>PQ9_8\" \">b;o"
    "g8=''`Q<`b8?'QQ9_8?c;dPe8dQa8<P]Q<``8=;'f8>QQ`]8?ocQQ_8=''_Q`8aQ\""
    " \"CYOUQWIN!]Q=;of8BQQ)88bc]QA98P'Qo]8>oQQ_8CP;QQ98b`_8@P8(Q`]'^Q="
    "`']QB)88\" \"bQQ]`8J;QQ`88bc99bd(`8iQ@''QQo^8E'`8c;QQ'Q``8?QQ`'kQ<"
    "o]8B9PQQ`';_Q<`_8<cbQ\" \"<o_Q<o_QBdo`QQ';`Q=9`]8=bobQ_o_Q<o]8=';b"
    "QC(88cQ`bcgQ>oP']8<;cQ>98)^Q?''oc`Q>\" \"oc']Q]8<;cQ>`8bbQA``QQ''`"
    "Q?88P;dQ=`)nQ?*/;;\" ;"
    "G(Z,60*(60*(10**s+s[1])+s[3]*10+s[4])+s[6]*10+s[7]-1933008 )G(l,(Z"
    "+86400-*o)%86400)G(M,o[3] *17+o[4]*11) G(O,(&x-&w)-(&y-&x)?1:0)G( "
    "m,*o)G(n,m)G(i,0)G(j,O)if(M>197)for( p=N;"
    "*p; )if(*p>92){*N=p[1 ];N[1] =0;"
    "for( O=*p++-90;0<O--;z( N));p++;"
    "}else{ s= p;for(O= *p++-59;O--;"
    "p++) p[-1]=*p;p[-1]=0;z(s);}else{"
    " if(M==0){*o=Z;l=0; }if(o[O+3]&&o[ O+1]+(O?3:5 )>l)Z=O;"
    "else++o[O+3];o[O+1] =l; if(l<60)d+= sprintf(d,\"#include<stdio.h>\""
    " \"\\12#define G(g,o)  int  g=o;\\n#d\" \"efine  \\161(b\" \")int\\"
    "40o[]={%d,%d,%d,%d,%d};char\" \"*p=#b,P[9999],*d=P,*e[]=\" \"{\\\""
    "%s\\\", \\\"\" \"%s\" \"\\\",\\\"%s\\\",\\\"%s\\\"},   b\\nq(%s)\\"
    "n\" ,*o,o[01],o[2],o[3], o[4] ,e[3],e[1], e[2],*e,p);"
    "z(\"/*;; \");for(i=0; i++<18 ;z(\";\"))for(j=0;"
    "j++<11;n=(n*193+287) %384 ){m=(97+(67*m ))%198;"
    "for (O=0;O++<n%03;z(\"Q\") );s=d;"
    "for(z(e[ ( n/3)&3]);m<M &&s-d;*s++ =040);"
    "}if(l<60) {for (z(\";Time:Q\") ;(l++<60);"
    "z (\"#\"));for (z(\";Heca\" \"t\" \"eQII:\" \"Q\") ;"
    "o[3 ]++ <=24;z(\"#\"));z (Z?\"\":\"Q(j\" \"a\" \"m\" \"me\" \"d\" "
    "\")\");"
    "for(z( \";GlockQ18\" \"C:\" \"Q\");o[4]++< 25;"
    "z(\"#\") );z( Z- 1? \";\":\"Q(jam\" \"med);\") ;"
    "} else z(\";\" \";Gam\" \"eQover.\" \";;\" );"
    "for (O= 0;O <77;O ++) z(\"Q\");z( \"*\" \"\" \"/\" );"
    "} puts (P);; return+0;}";

const char WIN_SCREEN[] =
    "\n"
    "               /*oooooooooo\n"
    "          oo88888888888888888boc\n"
    "       od888888888888888888888888oc\n"
    "     o888888888888888888888888888888oc\n"
    "    d8888888888888888888888888888888888o\n"
    "   d8888888888888888P88888888888888888c'''\n"
    "  d888888888888888P' )88888888889888888c\n"
    " d88888888888888P'   `888888888P 988888b\n"
    "o8888888888888''      `88888888'  988888c\n"
    "dP88888888888          8888888P   `888888\n"
    "'888888888888  `888oc  88888''     888888       YOU WIN!   \n"
    "o888888888888  )88bc   98P' o888o  88888P\n"
    "  98b`88888P8( `'''    `'   )88b  ]888888\n"
    "  `88bc99bd(`8               ''  o8888'`8c\n"
    "  ' `888888  `'                 o8889P  `'\n"
    "     `88888c        o     o     do`  '\n"
    "      9`888bo        ooooo     o888'\n"
    "        (88c `bc             oP'888\n"
    "         98)    ''oc      oc'   888\n"
    "         `8b        ``  ''      88P\n"
    "          `)                    */\n"
    "\n"
    "";

/*
 * These three functions are never called. Only their addresses matter, see
 * select_weapon(). They must stay together and in this order.
 */
extern void size_probe_loop()
{
    int countdown = start_time;

    while (countdown--) {
    }
}

extern void size_probe_empty_first()
{
}

extern void size_probe_empty_second()
{
}

/*
 * Without optimization the loop function is larger than the empty functions,
 * so the two address gaps differ and the Glock is selected. An optimizer that
 * deletes the empty loop makes the gaps equal, which selects the Hecate.
 *
 * Function placement also counts: gcc 14 puts append_text() right after the
 * loop function when optimizing, so the gaps differ there and it always picks
 * the Glock. append_text() is kept out of line to preserve that result.
 */
static enum weapon select_weapon(void)
{
    long first_gap = &size_probe_empty_first - &size_probe_loop;
    long second_gap = &size_probe_empty_second - &size_probe_empty_first;

    return first_gap - second_gap ? GLOCK : HECATE;
}

static __attribute__((noinline)) void append_text(const char *text)
{
    while ((*output_end = *text++) != 0) {
        output_end++;
    }
}

static int seconds_since_midnight(const char *clock_time)
{
    int hours = (clock_time[0] - '0') * 10 + (clock_time[1] - '0');
    int minutes = (clock_time[3] - '0') * 10 + (clock_time[4] - '0');
    int seconds = (clock_time[6] - '0') * 10 + (clock_time[7] - '0');

    return (hours * 60 + minutes) * 60 + seconds;
}

static void append_next_generation(void)
{
    output_end += sprintf(output_end,
        "#include<stdio.h>\n"
        "#define G(g,o)  int  g=o;\n"
        "#define  q(b)int o[]={%d,%d,%d,%d,%d};"
        "char*p=#b,P[9999],*d=P,*e[]={\"%s\", \"%s\",\"%s\",\"%s\"},   b\n"
        "q(%s)\n",
        start_time,
        last_shot_time[HECATE], last_shot_time[GLOCK],
        shots_fired[HECATE], shots_fired[GLOCK],
        enemy_faces[3], enemy_faces[1], enemy_faces[2], enemy_faces[0],
        NEXT_GENERATION_CODE);
}

/*
 * Two generators walk fixed sequences from the same seed. One sets the spacing
 * and face of each cell. The other gives each enemy a number from 0 to 197, and
 * an enemy is drawn as blanks once the damage dealt is above its number.
 */
static void append_enemy_grid(int seed, int damage)
{
    int enemy_number = seed;
    int cell_layout = seed;
    int row;
    int column;
    int padding;
    char *face_start;

    for (row = 0; row < GRID_ROWS; row++) {
        for (column = 0; column < GRID_COLUMNS; column++) {
            enemy_number = (97 + 67 * enemy_number) % ENEMY_COUNT;

            for (padding = 0; padding < cell_layout % 3; padding++) {
                append_text(" ");
            }

            face_start = output_end;
            append_text(enemy_faces[(cell_layout / 3) % FACE_COUNT]);
            if (enemy_number < damage) {
                while (face_start != output_end) {
                    *face_start++ = ' ';
                }
            }

            cell_layout = (cell_layout * 193 + 287) % 384;
        }
        append_text("\n");
    }
}

static void append_bar(int length)
{
    int position;

    for (position = 0; position < length; position++) {
        append_text("#");
    }
}

/*
 * jam_status holds the number of the jammed weapon. When no weapon jammed it
 * holds the time of day instead, so a shot at exactly 00:00:00 or 00:00:01
 * also displays as a jam, the same as the original.
 */
static void append_status(int elapsed, int jam_status)
{
    append_text("\nTime: ");
    append_bar(TIME_LIMIT - elapsed);

    append_text("\nHecate II: ");
    append_bar(AMMO_BAR_LENGTH - shots_fired[HECATE]);
    if (jam_status == HECATE) {
        append_text(" (jammed)");
    }

    append_text("\nGlock 18C: ");
    append_bar(AMMO_BAR_LENGTH - shots_fired[GLOCK]);
    if (jam_status == GLOCK) {
        append_text(" (jammed)");
    }
    append_text("\n");
}

int main()
{
    int now = seconds_since_midnight(__TIME__);
    int elapsed = (now + SECONDS_PER_DAY - start_time) % SECONDS_PER_DAY;
    int damage = shots_fired[HECATE] * weapon_damage[HECATE]
               + shots_fired[GLOCK] * weapon_damage[GLOCK];
    enum weapon weapon = select_weapon();
    int grid_seed = start_time;
    int jam_status = now;
    int padding;

    if (damage >= ENEMY_COUNT) {
        puts(WIN_SCREEN);
        return 0;
    }

    if (damage == 0) {
        start_time = now;
        elapsed = 0;
    }

    if (shots_fired[weapon] != 0
            && last_shot_time[weapon] + weapon_cooldown[weapon] > elapsed) {
        jam_status = weapon;
    } else {
        shots_fired[weapon]++;
    }
    last_shot_time[weapon] = elapsed;

    /*
     * The source code is printed only while time remains. The game display is
     * wrapped in a comment so that the printed file still compiles.
     */
    if (elapsed < TIME_LIMIT) {
        append_next_generation();
    }

    append_text("/*\n\n ");
    append_enemy_grid(grid_seed, damage);

    if (elapsed < TIME_LIMIT) {
        append_status(elapsed, jam_status);
    } else {
        append_text("\n\nGame over.\n\n");
    }

    for (padding = 0; padding < CLOSING_PADDING; padding++) {
        append_text(" ");
    }
    append_text("*/");

    puts(output);
    return 0;
}
