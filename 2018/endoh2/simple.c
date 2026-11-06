#include <stdio.h>

/*
 * The original program is a quine: it prints a new version of its own
 * source code, laid out in the shape of a party parrot.  Every new version
 * draws the next of ten parrots and shows the next of ten "dead parrot"
 * quotes on the last line.
 *
 * The original reads its own source through a stringizing macro, so the
 * text it sees has no spaces, no double quotes, no '!' and no ANSI color
 * codes.  The strings below are that text, cut into its parts.  They are
 * printed back unchanged (except for the frame counter), so they must stay
 * byte for byte the same as in the original.
 */

#define WIDTH  79
#define HEIGHT 24
#define CELLS  (WIDTH * HEIGHT)

/* C code that unpacks the macro text and runs the virtual machine. */
static const char decoder_code_part1[] =
    "for(;*Q>34?(*S++=*Q++):*(Q+=*Q-27?1:Q[2]/49+4););for(S=M+9;*C;u=t%9,"
    "t<18?*++S=t<0?*C++%92*57-t-500:t<9?M[u]:M[u]++:t<27?M[u]=*S--:u?t=*S"
    "--,u<2?M[t]=*S--:u<3?C+=*S--?t:0,0:u-6?*S=u<4?*S-t:u";
static const char decoder_code_part2[] =
    "<5?*S%t:*S<t:putchar(t):(*S=M[*S]))t=*C++-93;return+0**";

/* The comment put between the two parts of the decoder. */
static const char helmet_comment[] = "/*(HELMET)*/";

/*
 * Program for the virtual machine.  It does everything the C code in this
 * file does.  Only one character of it changes from frame to frame.
 */
static const char bytecode[] =
    "Ler[Tsaxiyj.V}<dzSGs`Fk{YezYTtbx0d{iykNT}?dzaxiyjEK}TdzHeiy`Me{T"
    "f|)dzHeiyHeiyjxiyaxLdz2sxKe{'d|Yd{2sy?[saY^{tGerjxujxvbqax`{)dzc"
    "kydj/dzjxkya_}JdziHf}Ydz-Vw2drQ)qJe_yh@*}<dzHehyAehy)*nx{qnxKe{D"
    "f|Qd{uc)e{&dzZfhyCehy6fuchyex;e{ZdzIe_ynq1dqD+u/dcyl5L}<dzLIETi{"
    "x=b{1g|{uc2d{byaxvcDfd}&dzDVd{x6ecOez1dcydCf_{{)d|qSf2T_{x$c{;g|"
    "{{uFfjx}4czbq_xucx/d}Jez0dlyc/d{hycSf{hychycFc{hy_)Ic}{q_0d{qb_}"
    ".cz:eo:T`{xp]q1d`{D`zD+ulxcx{-dzcCez7evOe~Hf~0dcx}/dzd~cxx~6f~m6"
    "e0dcx}{}Udzc(*{x~c0d{Tf|.dz'd~c5L}Ecz1d[Ty";

/*
 * Position of the frame counter in the bytecode.  The digit '1' means
 * frame 0, '0' frame 1, '/' frame 2 and so on down to '(' for frame 9.
 */
#define FRAME_DIGIT 398

/*
 * Table of start positions, colors and drawing directions used by the
 * bytecode, followed by the start of the next version of the program.
 */
static const char header[] =
    "/*0+)IFMNFU*+0-02;>@=,(#1555546231rst%wvu#08OCCC*/#include/*#exc"
    "lude*/<stdio.h>#define/**/Q(q)int*P,t,u,M[99999],*S=3557+M,*C=M+"
    "3800;char*Q=#q;int(main)(){q;}Q(";

/* The ten quotes, coded (see dead_parrot_quotes below). */
static const char encoded_quotes[] =
    "@@~cd`c/C@~o`rrdc~nm/GSghr~o`qqns~hr~mn~lnqd/FH~g`ud~bd`rdc~sn~a"
    "d/O@~dwohqdc~`mc~fnmd~sn~ldds~lx~l`jdq/GSghr~hr~`~k`sd~o`qqns/LA"
    "dqdes~ne~khed+~H~qdrs~hm~od`bd/KH&c~ad~otrghmf~to~sgd~c`hrhdr/X@"
    "~qtm~cnvm~sgd~btqs`hm~`mc~inhmdc~sgd~bgnhq~hmuhrhakd/FSghr~hr~`m"
    "~dw,o`qqns/";

/* The ten parrot outlines, compressed (see parrot_outlines below). */
static const char compressed_parrots[] =
    "6Zba4ORRZT`]Sa`fdgO2bPV[Ta5dT[MVfbZZHYb79bVT^:L;S<J=X>Q?X@SAZBWC"
    "IDNEQF`GXPUUgW^aa[bIaZ[Zafaf_`Kg`ZYY[[`gff^Jg``8OKKT,VNa6/TZSW`0"
    "cRMWO33f][T`-[KaV,[Z/6aPWLabf[P[a3f`0dPR`3M3]8/R[Z-Z,VZR#MRTfNTT"
    "4d5^$S]8RMKRVaMM8T$9aaZgaRSg1NZ1`5^d`LK].%&VZ,'R#gKaZf1TT10aa`b]"
    ".TZ-KK/28`-6VKf'ZH^P(./Ka8a)8[O6NaTbf*[Hd+Z(V8&3K%)VNTR#&WbX[H^d"
    "cOWOf]8[OR[aTOM2V'$9aLR[RW*`0+OMW[([14ZLaK%).NO6RMRS*[0+ZOWZS]";

static const char trailer[] =
    "_<<<<<http://cultofthepartyparrot.com/>>>>>__<-_Source_of_Parrot"
    "s__(";

/*
 * The decoded quotes.  In encoded_quotes each quote is a column character
 * followed by its letters, each one less than the real letter ('~' is a
 * space and '@' is "I'm"), and ends with '/'.  The '!' is added later.
 */
static const char *dead_parrot_quotes[10] = {
    "I'm dead",
    "I'm passed on",
    "This parrot is no more",
    "I have ceased to be",
    "I'm expired and gone to meet my maker",
    "This is a late parrot",
    "Bereft of life, I rest in peace",
    "I'd be pushing up the daisies",
    "I'm run down the curtain and joined the choir invisible",
    "This is an ex-parrot",
};

/*
 * The decoded parrot outlines, made by undoing the byte pair encoding of
 * compressed_parrots.  They are turtle drawing commands:
 *   '0'..'7'  mark the current cell as outline, turn clockwise by that many
 *             eighths of a circle and step one cell forward
 *   '<' '>'   jump 4 cells left or right without drawing
 */
static const char *parrot_outlines[10] = {
    "000001717110010001717107100000000170012<27007<1666>66007170770717006"
    ">066612017101717007070<<<<<<<<<<43010710717162626",
    "00017026100100100001710710000001701712<27007<1666>66007170770717006"
    ">066612161072712600610016260<<<<<<<<<<44610106207162",
    "00000170171010017261171710710000000017012<27007<1666>660071707707170"
    "06>06661217071010176170001701717174<<<<<<<<<<<21007",
    "0000017010711000100001071071000001701721<27007<1666>6600717077071700"
    "6>0666111177101007701700170017017000<<<<<<<<<<<4027100",
    "00000017171710101000017171071000017017012<27007<1666>660071707707170"
    "06>06661217710011760170001701701700<<<<<<<<<<41102",
    "0000000171726110010001710710000000001702612<27007<1666>6600717077071"
    "7006>066612000010061701717<<<<<<<<<610",
    "000000017110010000017100710000017001712<27007<1666>66007170770717006"
    ">06661217001017260600<<<<<<<<447170",
    "000000000170101621001717171007100001700017012<27007<1666>66007170770"
    "717006>0666121701010726600<<<<<<<<<43171071716",
    "00000170017101001017171071071000000001701721<27007<1666>660071707707"
    "17006>0666112701000107060<<<<<<<<<<44717107107000",
    "00017001717172711001717171007100000017001712<27007<1666>660071707707"
    "17006>06661201071010070600<<<<<<<<<<447171710717007",
};

/* Where each parrot's outline starts on the screen. */
static const int parrot_start_row[10] = { 19, 19, 19, 21, 21, 21, 21, 21, 21, 21 };
static const int parrot_start_col[10] = { 39, 34, 30, 26, 23, 25, 28, 37, 39, 42 };

/* ANSI color of each parrot's body: red, yellow, green, cyan, blue, magenta. */
static const char parrot_color[10] = "1326455551";
#define OUTLINE_COLOR '0'   /* black */

/* The eight directions in clockwise order, starting with "left". */
static const int step[8] = {
    -1,              /* left       */
    -WIDTH - 1,      /* up-left    */
    -WIDTH,          /* up         */
    -WIDTH + 1,      /* up-right   */
    1,               /* right      */
    WIDTH + 1,       /* down-right */
    WIDTH,           /* down       */
    WIDTH - 1,       /* down-left  */
};

enum cell { OUTLINE, BODY, OUTSIDE };

static char text[CELLS];
static enum cell canvas[CELLS];
static int fill_stack[CELLS * 4];

/* Lay out the next version of the program as WIDTH x HEIGHT characters. */
static void build_text(int next_frame)
{
    const char *pieces[] = { bytecode, header, encoded_quotes,
                             compressed_parrots, trailer };
    unsigned i, j;
    int pos = 0;

    for (i = 0; header[i]; i++)
        text[pos++] = header[i];
    for (i = 0; decoder_code_part1[i]; i++)
        text[pos++] = decoder_code_part1[i];
    for (i = 0; helmet_comment[i]; i++)
        text[pos++] = helmet_comment[i];
    for (i = 0; decoder_code_part2[i]; i++)
        text[pos++] = decoder_code_part2[i];

    /* everything else goes inside one long string literal */
    text[pos++] = '"';
    for (j = 0; j < sizeof pieces / sizeof pieces[0]; j++) {
        for (i = 0; pieces[j][i]; i++) {
            /* end the literal at the end of a row and open a new one */
            if ((pos + 1) % WIDTH == 0) {
                text[pos++] = '"';
                text[pos++] = '"';
            }
            if (pieces[j] == bytecode && i == FRAME_DIGIT)
                text[pos++] = '1' - next_frame;
            else
                text[pos++] = pieces[j][i];
        }
    }
}

/* Put the frame's quote in the middle of the last row: "(  quote!  ") */
static void write_quote(int frame)
{
    char *row = text + CELLS - WIDTH;
    const char *quote = dead_parrot_quotes[frame];
    int len = 0, col;

    while (quote[len])
        len++;
    for (col = 2; col < WIDTH - 2; col++)
        row[col] = ' ';
    row[WIDTH - 2] = '"';
    row[WIDTH - 1] = ')';

    col = (WIDTH - (len + 1)) / 2;
    for (; *quote; quote++) {
        row[col++] = *quote;
    }
    row[col] = '!';
}

/* Draw the outline of a parrot and fill its body. */
static void draw_parrot(int frame)
{
    const char *cmd;
    int start = parrot_start_row[frame] * WIDTH + parrot_start_col[frame];
    int cell = start;
    int dir = 0;
    int top = 0;
    int i;

    for (i = 0; i < CELLS; i++)
        canvas[i] = OUTSIDE;

    for (cmd = parrot_outlines[frame]; *cmd; cmd++) {
        if (*cmd == '<') {
            cell -= 4;
        } else if (*cmd == '>') {
            cell += 4;
        } else {
            canvas[cell] = OUTLINE;
            dir = (dir + *cmd - '0') % 8;
            cell += step[dir];
        }
    }

    /* flood fill the body, starting just right of the start point */
    fill_stack[top++] = start + 1;
    while (top > 0) {
        cell = fill_stack[--top];
        if (cell < 0 || cell >= CELLS || canvas[cell] != OUTSIDE)
            continue;
        canvas[cell] = BODY;
        fill_stack[top++] = cell - 1;
        fill_stack[top++] = cell - WIDTH;
        fill_stack[top++] = cell + 1;
        /* never spread down into the last row */
        if (cell + WIDTH < CELLS - WIDTH)
            fill_stack[top++] = cell + WIDTH;
    }
}

/* Print the text, coloring it with ANSI escape codes from the canvas. */
static void print_frame(int frame)
{
    enum cell previous = OUTSIDE;
    int i;

    for (i = 0; i < CELLS; i++) {
        if (canvas[i] != previous) {
            if (canvas[i] == OUTSIDE) {
                printf("\033[0m");
            } else {
                char color = canvas[i] == OUTLINE ? OUTLINE_COLOR
                                                  : parrot_color[frame];
                printf("\033[3%cm\033[4%cm", color, color);
            }
            previous = canvas[i];
        }
        putchar(text[i]);
        if (i % WIDTH == WIDTH - 1)
            putchar('\n');
    }
}

int main()
{
    int frame = '1' - bytecode[FRAME_DIGIT];
    int next_frame = (frame + 1) % 10;

    build_text(next_frame);
    write_quote(frame);
    draw_parrot(frame);
    print_frame(frame);
    return 0;
}
