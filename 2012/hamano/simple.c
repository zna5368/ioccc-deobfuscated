#include <stdio.h>

/*
 * Reads text from stdin and writes a one page PDF to stdout. Letters are
 * drawn as "dancing men" glyphs from a Type3 font built into the PDF, every
 * other character uses Courier.
 *
 * The output is also a valid C program. Everything that is PDF but not C is
 * hidden inside C comments, and the PDF content stream (the page text) is
 * hidden inside a C string literal that the C program prints back out.
 */

/* print and keep a running count of the bytes written so far */
#define EMIT(...) (offset += printf(__VA_ARGS__))

#define FIRST_CHAR 'A'
#define LAST_CHAR 'z'
#define NUM_OBJECTS 64        /* PDF objects 0..63 */
#define GLYPH_OBJECT(c) ((c) - 59) /* 'A' -> object 6 ... 'z' -> object 63 */

/* position in the output, counted from the "%PDF" header */
unsigned offset;

/* byte offset of every PDF object, for the cross reference table */
unsigned object_offset[NUM_OBJECTS];

/*
 * Pose of the dancing man for each letter (upper and lower case share one):
 *   bits 0-1  leg on the right of the picture
 *   bits 2-3  leg on the left of the picture
 *   bits 4-5  arm on the right of the picture
 *   bits 6-7  arm on the left of the picture
 *   bit  8    man stands on his head (picture is flipped upside down)
 */
int dancer_pose[26] = {84,  0,   64,  282, 90,  74,  330, 85, 93,
                       173, 167, 176, 80,  208, 81,  13,  7,  87,
                       160, 346, 32,  128, 170, 218, 16,  26};

/*
 * Limbs as polylines on a 10x10 grid, written as "x1y1x2y2..." digit pairs.
 * The hip is at (5,4) and the shoulder at (5,6)/(5,7).
 */
char *right_leg[4] = {"54726070", "546070", "548090", "549495"};
char *left_leg[4] = {"54233121", "544030", "542010", "541415"};
char *right_arm[4] = {"", "5689", "567689", "577654"};
char *left_arm[4] = {"", "5629", "563629", "573654"};

/*
 * Append a polyline to the glyph stream as "x y m x y l x y l ".
 * When upside_down is set every y coordinate is mirrored (y -> 9 - y).
 */
char *add_polyline(char *out, char *points, int upside_down) {
  int i;
  for (i = 0; points[i]; i += 2) {
    int x = points[i] - '0';
    int y = points[i + 1] - '0';
    if (upside_down)
      y = 9 - y;
    out += sprintf(out, "%d %d %c ", x, y, i == 0 ? 'm' : 'l');
  }
  return out;
}

/* Build the drawing commands of the Type3 glyph for character c */
int make_glyph(char *glyph, int c) {
  int index = c >= 'a' ? c - 'a' : c - 'A';
  int pose = index < 26 ? dancer_pose[index] : 0;
  int upside_down = pose >> 8;
  char *out = glyph;

  out += sprintf(out, "10 0 d0 "); /* glyph is 10 units wide */
  out = add_polyline(out, left_arm[pose >> 6 & 3], upside_down);
  out = add_polyline(out, right_arm[pose >> 4 & 3], upside_down);
  out = add_polyline(out, left_leg[pose >> 2 & 3], upside_down);
  out = add_polyline(out, right_leg[pose & 3], upside_down);

  /* upper case letters carry a small flag (in the story: end of a word) */
  if ((c & 32) == 0)
    out += sprintf(out, "8 7 2 2 re ");

  /* torso, then round line style and stroke everything drawn so far */
  out = add_polyline(out, "5458", upside_down);
  out += sprintf(out, "1 j 1 J 1 w s   ");
  /* head is a zero length line stroked with a wide round pen */
  out = add_polyline(out, "5959", upside_down);
  out += sprintf(out, "2 w s ");

  return out - glyph;
}

/* start PDF object number n and remember where it is */
void begin_object(int n) {
  object_offset[n] = offset;
  EMIT("%u    0  obj", n);
}

int main(void) {
  char glyph[256];
  int c, length;
  unsigned stream_start, stream_length, xref_start;

  /*
   * "/" "*" opens a C comment, "%PDF-1.3" is the PDF header and the following
   * "%" makes "*" "/" a PDF comment that closes the C comment. Offsets in the
   * PDF are counted from "%PDF", 2 bytes in.
   */
  EMIT("/*%%PDF-1.3%%*/");
  offset -= 2;

  /*
   * C code that is the PDF's "garbage" before the first object. The macros
   * turn the later PDF keywords "endstream" and "endobj" into a main()
   * function that prints every character found between "(" and ")" in the
   * string _ and a newline for every empty "()".
   */
  EMIT("#include<stdio.h>\n");
  EMIT("#define o *_++&& *_-41\n#define");
  EMIT(" endstream ");
  EMIT("main(){for(;*_++; *_-40?:putchar(o?*_:o?10:41));\n");
  EMIT("#define endobj return 0;}\n");
  /* "ET" (PDF end text) becomes the type of main(), then "/" "*" hides obj 1 */
  EMIT("typedef int ET;/");
  EMIT("*");

  /* object 1: the page content stream */
  begin_object(1);
  EMIT("<</Length 2 0 R>>stream\n");
  stream_start = offset;

  /*
   * Begin text at the top left, 12pt font, 12pt leading. The PDF comment
   * "%" lets "*" "/" close the C comment and the C string literal _ starts.
   */
  EMIT("BT 12 818 Td/U 12 Tf 12 TL%%*/static char*_=\" \\\n");

  while ((c = getchar()) != EOF) {
    if (c == '\n')
      EMIT("()'"); /* empty string on a new line */
    else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
      EMIT("/T 12 Tf(\\%o)Tj", c); /* dancing men font */
    else
      EMIT("/U 12 Tf(\\%o)Tj", c); /* Courier font */
  }
  /* "%" hides the end of the C string from the PDF */
  EMIT("%%\";\nET ");
  stream_length = offset - stream_start;
  /* the rest of the PDF is hidden in a C comment */
  EMIT("endstream endobj /*");

  /* objects 6..63: one Type3 glyph program per character 'A'..'z' */
  for (c = FIRST_CHAR; c <= LAST_CHAR; c++) {
    length = make_glyph(glyph, c);
    begin_object(GLYPH_OBJECT(c));
    EMIT("<</Length %d>>stream\n%sendstream endobj ", length, glyph);
  }

  /* object 2: length of the content stream */
  begin_object(2);
  EMIT(" %u endobj ", stream_length);

  /* object 3: document catalog */
  begin_object(3);
  EMIT("<</Pages 4 0 R>> endobj ");

  /* object 4: page tree, one A4 page */
  begin_object(4);
  EMIT("<</Count 1/MediaBox[0 0 595 842]/Kids[5 0 R]>> endobj ");

  /* object 5: the page and its two fonts, U (Courier) and T (dancing men) */
  begin_object(5);
  EMIT("<</Type/Page         /Parent %d %d R /Resources <<        /ProcSet[  "
       "     /PDF/Text]/Font       <</U<</Subtype/Type1/BaseFont /Courier>>  "
       "/T<</Subtype/Type3   /FontBBox[0 0 10 10]/FontMatrix[           %f 0 "
       "0 %f 0 0]/FirstChar %d/LastChar %d/Encoding<</Differences[%d",
       4, 0, 0.06, 0.06, FIRST_CHAR, LAST_CHAR, FIRST_CHAR);

  /* glyph names: the letters themselves, the 6 symbols between Z and a
     reuse the glyph of A */
  for (c = FIRST_CHAR; c <= LAST_CHAR; c++)
    EMIT("/%c", c > 'Z' && c < 'a' ? FIRST_CHAR : c);
  EMIT("]>>/CharProcs<<");
  for (c = FIRST_CHAR; c <= LAST_CHAR; c++)
    if (!(c > 'Z' && c < 'a'))
      EMIT("/%c %d 0 R", c, GLYPH_OBJECT(c));

  /* every one of the 58 characters is 10 units wide */
  EMIT(">>/Widths[");
  for (c = FIRST_CHAR; c <= LAST_CHAR; c++)
    EMIT(" 10");
  EMIT("]>>>>>>/Contents 1                              0 R>> endobj ");
  xref_start = offset;

  /* cross reference table (entry 0 holds the stream length by accident) */
  EMIT("xref 0 %d ", NUM_OBJECTS);
  EMIT("%010u %05d n  ", stream_length, 0);
  for (c = 1; c < NUM_OBJECTS; c++)
    EMIT("%010u %05d n  ", object_offset[c], 0);

  /* trailer, then "*" "/" closes the C comment */
  EMIT("trailer   <</Root 3 0 R /Size %d>>\nstartxref %u %%%%EOF\n*/",
       NUM_OBJECTS, xref_start);
  return 0;
}
