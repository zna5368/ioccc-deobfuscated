/*
 * The data string holds two regions:
 *
 *   data[0 .. LAYOUT_SIZE-1]   layout commands for four pictures, one after
 *                              another.  The first byte of each picture's
 *                              commands (here 'X') tells how many commands
 *                              it has: data[0] + LAYOUT_SKEW - 1.
 *
 *   data[LAYOUT_SIZE ..]       the C code of the next program, with every
 *                              byte XORed with CODE_KEY, ending in a '\0'.
 *
 * Spaces in either region are only there to draw the picture and are skipped.
 */
char *data = "X0[!4cM,!"
    "4cK`*!4cJc(!4cHg&!4c$j"
    "8f'!&~]9e)!'|:d+!)rAc-!*m*"
    ":d/!4c(b4e0!1r2e2!/t0e4!-y-c6!"
    "+|,c6!)f$b(h*c6!(d'b(i)d5!(b*a'`&c"
    ")c5!'b+`&b'c)c4!&b-_$c'd*c3!&a.h'd+"
    "d1!%a/g'e+e0!%b-g(d.d/!&c*h'd1d-!(d%g)"
    "d4d+!*l,d7d)!,h-d;c'!.b0c>d%!A`Dc$![7)35E"
    "!'1cA,,!2kE`*!-s@d(!(k(f//g&!)f.e5'f(!+a+)"
    "f%2g*!?f5f,!=f-*e/!<d6e1!9e0'f3!6f)-g5!4d*b"
    "+e6!0f%k)d7!+~^'c7!)z/d-+!'n%a0(d5!%c1a+/d4"
    "!2)c9e2!9b;e1!8b>e/!     7cAd-!5fAe+!7fBe(!"
    "8hBd&!:iAd$![7S,Q0!1     bF 7!1b?'_6!1c,8b4"
    "!2b*a,*d3!2n4f2!${4    f.      '!%y4e5!&f%"
    "d-^-d7!4c+b)d9!4c-a    'd        :!/i('`&d"
    ";!+l'a+d<!)l*b(d=!'   m-        a  &d>!&d'"
    "`0_&c?!$dAc@!$cBc@!$   b         <    ^&d$`"
    ":!$d9_&l++^$!%f3a'    n1        _       $ !&"
    "f/c(o/_%!(f+c)q*c     %!         *       f &d+"
    "f$s&!-n,d)n(!0i-     c-         k)       !  3d"
    "/b0h*!H`7a,![7*     i]          5        4   71"
    "[=ohr&o*t*q*`*d      *v         *r         ;  02"
    "7*~=h./}tcrsth      &t          :          r   9b"
    "].,b-725-.t--//      #r         [           <   t8-"
    "752793?  <.~;b      ].t--+r     /           #    53"
    "7-r[/9~X  .v90      <6/<.v;-52/={            k   goh"
    "./}q;   u  vto     hr  `.i*$engt$            $    ,b"
    ";$/     =t ;v;     6     =`it.`;7=`          :    ,b-"
    "725    = / o`.    .d       ;b]`--[/+       55/     }o"
    "`.d   :   - ?5    /           }o`.'     v/i]q      - "
    "-[;   5  2  =`  it            .        o;53-       . "
    "v96   <7 /      =o            :            d        =o"
    "--/i  ]q--      [;           h.            /        = "
    "i]q--[  ;v      9h           ./            <        - "
    "52={cj   u      c&`          i   t       . o        ; "
    "?4=o:d=         o--          /  i        ]q         - "
    "-[;54={  cj     uc&          i]q          -          -"
    "[;76=i]q[;6     =vsr        u.i           /          ={"
    "=),BihY_gha     ,)\0        ";

#define LAYOUT_SIZE 768   /* bytes of picture layout commands (with spaces) */
#define CODE_SIZE 947     /* bytes of encoded code region (with spaces, '\0') */
#define LAYOUT_SKEW 143   /* data[0] + LAYOUT_SKEW = end of current commands */
#define LAST_POS 2659     /* after this, only ';' fillers are produced */
#define CODE_KEY 6        /* XOR key used to hide the code text */

#define CMD_NEWLINE '!'   /* layout command: end of line */
#define CMD_FIRST_GAP 93  /* commands >= 93 are runs of (cmd - 92) spaces */

char output[3217];
int read_pos;   /* position in the sequence of characters to emit */
int in_code;    /* 0 while writing the string literal, '"' after */

/*
 * Produce the next character of the new program's text:
 *   1. the layout commands rotated so the next picture comes first,
 *   2. the encoded code, still encoded (inside the string literal),
 *   3. a '"' when the '\0' is reached, which closes the string literal,
 *   4. the code again, this time decoded,
 *   5. ';' fillers once everything has been written.
 */
next_char() {
  int ch;

  if (read_pos < LAYOUT_SIZE)
    return data[(LAYOUT_SKEW + read_pos++ + data[0]) % LAYOUT_SIZE];
  if (read_pos > LAST_POS)
    return ';';

  ch = data[(read_pos++ - LAYOUT_SIZE) % CODE_SIZE + LAYOUT_SIZE];
  if (ch != 0)
    return ch ^ (in_code ? CODE_KEY : 0);
  return in_code = '"';
}

/* Same as next_char() but skips the decorative spaces of this source. */
next_visible_char() {
  int ch;

  do
    ch = next_char();
  while ((ch ^ (in_code ? CODE_KEY : 0)) == ' ');
  return ch;
}

int main() {
  int cmd_pos, cmd, i, count;
  int len;

  read_pos = in_code = 0;
  len = sprintf(output, "char*d=");

  for (cmd_pos = 1; cmd_pos < data[0] + LAYOUT_SKEW;) {
    cmd = data[cmd_pos++];

    if (cmd == CMD_NEWLINE) {
      output[len++] = '\n';
    } else if (cmd >= CMD_FIRST_GAP) {
      /* a gap of blank cells */
      for (i = 0; i < cmd - (CMD_FIRST_GAP - 1); i++)
        output[len++] = ' ';
    } else {
      /* a run of (cmd - 34) filled cells */
      if (!in_code) {
        /* inside the string literal: wrap the run in quotes */
        output[len++] = '"';
        count = cmd - 36;
      } else {
        count = cmd - 35;
      }
      for (i = 0; i < count; i++)
        output[len++] = next_visible_char();
      output[len++] = in_code ? next_visible_char() : '"';
    }
  }

  output[len] = 0;
  puts(output);
}
