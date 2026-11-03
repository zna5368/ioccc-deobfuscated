#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Tengwar vector font.
 *
 * Every glyph is a string of drawing commands.  The pen is a broad nib with
 * two edges; each "pen position" gives the location of both nib edges.
 *
 *   XXXXXX  six characters with the first one > '<' : a pen position.
 *           Each group of three characters encodes one (x, y) point:
 *             y = (c0 % 64) * 8 | (c1 / 8) & 7
 *             x = (c1 % 8) * 64 + c2 % 64 - 256
 *           The first point is the left nib edge, the second the right one.
 *   =       directly after a pen position: this position starts a new stroke.
 *   :       end of the current stroke.
 *   8c      move the pen to the right by (c % 64) * 4 pixels.
 *
 * The pen positions of a stroke are joined with Catmull-Rom splines and the
 * area swept by the nib is filled in.
 */

/* Shared pieces used by several tengwar */
#define TAIL_DOWN "Hk~HdA=Jk|Jk~LSyL[{M[wMcxNksNss:"
#define WIDE_ARCH                                                              \
  "Ht@H|@=HdJHtJHdYHtY:HtFHtF=JDBIlDJTEJDFIlMIlM:HdMHdM=I|KIlMJTOJDOIlWITY:8Y"
#define DOUBLE_CLOSED_BOW                                                      \
  "IT@I\\@=HdHHtGH|KILJJDIJDH:H|KIDK=HdQHtPH|TIDRJDRJDQ:JC?JK?=JDRJLRI|UItU:8T"
#define OPEN_BOW "IS?I\\@=HdGHtGIDJILIJDIItHJTFJDF:8J"
#define U_CURL                                                                 \
  "G\\QG\\P=GLPGTPGdMGdNGtOGlOGdSGdRGDPGLPG\\LG\\LHtGHtH:"
#define TALL_STEM "Hs?H{?=HdGH|FI\\II\\GJlHJlFL\\DLTCMlAM\\@Ns}Nk|:8G"
#define EXTENDED_STEM                                                          \
  "EDGEDH=EtCElDH{~H|AJk}Jk?LSzL[|M[wMcxNksNst:"
#define SHORT_STEM "Hs?H|@=HdFHtEI\\HI\\FJLHJTD:8H"

/*
 * Glyph table.  Lower case letters are single sounds; the upper case keys
 * are produced by digraphs ending in 'h' (gh, mh, rh, sh, th, uh, zh).
 */
struct glyph {
  char key;
  const char *strokes;
};

static const struct glyph font[] = {
    {',', "8O"},
    {'.', "8O"},
    {'G', WIDE_ARCH EXTENDED_STEM},
    {'M', SHORT_STEM DOUBLE_CLOSED_BOW},
    {'R', SHORT_STEM OPEN_BOW},
    {'S', "8CHdDH|E=HtAIDAIt@IlAJTCJDCIlKI\\K:8K" EXTENDED_STEM},
    {'T', "DdWDdW=D\\UD\\VF\\FFdHGtCGtEIDBIDDIlBIdDJT@JLC:8D" OPEN_BOW},
    {'U', "GDNG\\L=GDJGLKHLFHLGHtEHtE:" U_CURL},
    {'Z', "FDTFLT=G|EGlHITBH|DIlDIdE:HtMH|M=JDBJLDKLAKDALDFKtFKdMK\\LJTOJ\\NJT"
          "MJTM:8M"},
    {'a', "GtFGlG=G|HG|H:G\\IG\\J=G|IG|I:GdKGlL=G|JG|J:"},
    {'b', TALL_STEM DOUBLE_CLOSED_BOW},
    {'d', TALL_STEM OPEN_BOW OPEN_BOW},
    {'g', WIDE_ARCH TAIL_DOWN},
    {'i', "GlIGlK=G|JG|J:"},
    {'k', "Hl@Ht@=HdDHtCHdPH|P:HdDHdD=ItBIlDJTEJDFIdNI\\N:8N" TAIL_DOWN},
    {'l', "ID@IL@=HlIH|FHlPH|NHt^H|^:H|MH|N=J\\DJ\\GK\\OKTOKDXJtXItZI|YIlWI|V:"
          "8^"},
    {'m', "HLGH\\G=HLVH\\V:"},
    {'n', SHORT_STEM OPEN_BOW OPEN_BOW},
    {'p', TALL_STEM
     "IT@I\\@=HdHHtGIDKILIJLGJLG:JK?JK?=JDGJLGI|MJDL:8M"},
    {'r', "Ht@H|@=HtDH|BJdLJTH:ITEI\\E=ILPILNNtCNlB:8N"},
    {'t', TALL_STEM OPEN_BOW},
    {'u', U_CURL},
    {'z', "I[?Il@=HlHH|HIDLILIJDII|HKDAJ|A:JtCJtC=JdLJtJLTHLdFNk|Nc|:8K"},
};

#define HEIGHT 640     /* canvas height (output is half of this) */
#define LINE_STEPS 16  /* dots drawn across the nib */
#define CURVE_STEPS 512 /* samples along each spline segment */

/* nib coordinates: left edge x, left edge y, right edge x, right edge y */
enum { LEFT_X, LEFT_Y, RIGHT_X, RIGHT_Y, COORDS };

static double pen_x = 32; /* horizontal pen position */
static char (*canvas)[HEIGHT]; /* canvas[x][y], 1 = ink */

/* The last four pen positions; point[3] is the newest */
static int point[4][COORDS];
/* Cubic polynomial coefficients for each coordinate, coef[power][coord] */
static int coef[4][COORDS];

static const struct glyph *find_glyph(char key) {
  size_t n;
  /* quirk: " h" turns into key 0, which the original matched to the
     first glyph (it searched the font for an empty key) */
  if (key == '\0')
    return &font[0];
  for (n = 0; n < sizeof font / sizeof font[0]; n++)
    if (font[n].key == key)
      return &font[n];
  return NULL;
}

/* Forget the oldest pen position, the newest one stays in point[3] */
static void shift_points(void) {
  int c;
  for (c = 0; c < COORDS; c++) {
    point[0][c] = point[1][c];
    point[1][c] = point[2][c];
    point[2][c] = point[3][c];
  }
}

/* Decode three font characters into one point */
static const char *read_point(const char *s, int *x, int *y) {
  *y = s[0] % 64 * 8;
  *y |= s[1] / 8 & 7;
  *x = s[1] % 8;
  *x = *x * 64 + s[2] % 64 - 256;
  return s + 3;
}

/* Catmull-Rom spline coefficients (times two) between point[1] and point[2] */
static void compute_coefficients(void) {
  int c;
  for (c = 0; c < COORDS; c++) {
    int p0 = point[0][c], p1 = point[1][c], p2 = point[2][c], p3 = point[3][c];
    coef[0][c] = 2 * p1;
    coef[1][c] = p2 - p0;
    coef[2][c] = 2 * p0 - 5 * p1 + 4 * p2 - p3;
    coef[3][c] = 3 * p1 - p0 - 3 * p2 + p3;
  }
}

static double spline(int c, double t) {
  return (((coef[3][c] * t + coef[2][c]) * t + coef[1][c]) * t + coef[0][c]) *
         2;
}

/* Sweep the nib along the current spline segment, from t = 1 down to 0 */
static void draw_segment(void) {
  double t = 1;
  while (t) {
    double x, y, dx, dy;
    int step;

    t -= 1.0 / CURVE_STEPS;
    /* line from the left nib edge to the right nib edge */
    x = spline(LEFT_X, t);
    dx = (spline(RIGHT_X, t) - x) / LINE_STEPS;
    y = spline(LEFT_Y, t);
    dy = (spline(RIGHT_Y, t) - y) / LINE_STEPS;
    for (step = 0; step < LINE_STEPS; step++) {
      x += dx;
      y += dy;
      canvas[(int)(x + pen_x)][(int)y] = 1;
    }
  }
}

static void draw_glyph(const char *s) {
  for (;;) {
    if (*s > '<') {
      /* next pen position */
      shift_points();
      s = read_point(s, &point[3][LEFT_X], &point[3][LEFT_Y]);
      s = read_point(s, &point[3][RIGHT_X], &point[3][RIGHT_Y]);
      if (*s == '=') {
        /* start of a stroke: all four spline points are this position */
        s++;
        shift_points();
        shift_points();
        shift_points();
      }
      compute_coefficients();
      draw_segment();
    } else if (*s > '9') {
      /* end of a stroke: repeat the last position to finish the spline */
      s++;
      shift_points();
      compute_coefficients();
      draw_segment();
    } else if (*s > '6') {
      /* advance the pen */
      s++;
      pen_x += *s++ % 64 * 4;
    } else {
      break;
    }
  }
}

int main(int argc, char **argv) {
  char *text = argv[1];
  int width, x, y;

  (void)argc;
  canvas = calloc(strlen(text) + 2, 256 * HEIGHT);

  while (*text) {
    char key = *text++;
    const struct glyph *g;

    /* a following 'h' selects the upper case (digraph) form */
    if (*text == 'h') {
      key ^= 32;
      text++;
    }
    g = find_glyph(key);
    if (g)
      draw_glyph(g->strokes);
  }

  /* write a PGM image, each pixel is the sum of a 2x2 block (0..4) */
  width = pen_x / 2 + 32;
  printf("P2 %d %d 4 ", width, HEIGHT / 2);
  for (y = 0; y < HEIGHT; y += 2)
    for (x = 0; x < width * 2; x += 2)
      printf("%d ", canvas[x][y] + canvas[x][y + 1] + canvas[x + 1][y] +
                        canvas[x + 1][y + 1]);
  return 0;
}
