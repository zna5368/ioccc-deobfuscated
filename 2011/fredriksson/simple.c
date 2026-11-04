#include <stdio.h>
void exit(int);
int system(char *);

/*
 * The program carries the source code of the real program (an approximate
 * grep, written to ag.c) together with the file name and the shell command
 * that builds and runs it.  All of it is packed into one string:
 *
 *   1. The plain text was Burrows-Wheeler transformed.
 *   2. The result was run-length encoded in unary: a run of N equal
 *      characters is written as N-1 spaces followed by the character.
 *      A literal space is written as '@' and a NUL byte as '$'.
 */
#define PAYLOAD_LENGTH 2099 /* characters in the transformed text */
#define BWT_PRIMARY_INDEX 1601 /* row of the original text in the BWT matrix */
#define ALPHABET_SIZE 128 /* the payload only uses 7-bit ASCII */

/* Offsets of the pieces inside the decoded text */
#define FILE_NAME_OFFSET 0 /* "ag.c" */
#define FORMAT_OFFSET 5 /* "%.*s" */
#define COMMAND_OFFSET 10 /* "gcc -O3 ag.c -o ag;./ag" */
#define SOURCE_OFFSET 35 /* source code of ag.c, up to the end */
#define SOURCE_LENGTH (PAYLOAD_LENGTH - SOURCE_OFFSET)

int symbol_count[ALPHABET_SIZE];   /* how often each character occurs */
int first_position[ALPHABET_SIZE]; /* where each character starts in the
                                      sorted first column of the matrix */
int earlier_count[PAYLOAD_LENGTH]; /* same characters before position i */
char decoded[PAYLOAD_LENGTH];

/* The packed payload.  After decoding, this buffer is reused for the shell
   command that is passed to system(). */
char payload[] =
    "@cs                                                             "
    " \"$ | ,&    |& (&)x ) &,(   ,(| ,(    ,{      |,},;   ,( ;,>   "
    ",(  ,( ,;,       );g cgt3odt  -  <~+<+-       +            ~ <&~"
    "<~+ <&+~+  ~ <=-  =       (  -                                  "
    "                            !()  c  !(&(                        "
    "                                      !                         "
    "                                     \b\n$P    & m& )P )o )e')\""
    ")\"' ) w_ 'w)e ')x') ')q 'a_ )x'\"up)q                          "
    "       &]v\")    &t  &t-k[]t\\.nivc -:l O   CI:OnO lCl  ={ -: =("
    "-      +[=+        =C         =]\\.[-       & -   ~ -(|,g& -&-  "
    "      <- ( ~ -g()=  Q| b)  *,t,       Q-            ( -   (\n& ~"
    "  &      Q  \n &\n,&g r& \n&   \n  ,&~\nt &=\n &n) rt)+=\n&  (~ "
    " &k=  &g(-&=  (Q -(  P  ) \n )\n  ) \"+\"' )z\"ujlpx)\"*bgQ Px )"
    "(\")x)\" P     \"P\"'i x  )\")m') axP            \")\"  Pv    pP"
    ")e  )O\"_\"apvP(t(c  )d) \")\"pqpmpleP  \"*te\" t\n \"]   \"O_\""
    "  g c *P      )\" =+   *   r\n&r\nr     \n! *  (*  (*      +    "
    "  *.            )x      ')+)      +)*    ,\n )&* +  ma +) 'j    "
    " )d)' m)O )e)nc  )m )n         ) ')\"'jl   _) _)_  )nps' )cix'im"
    "l )tit_eqp)m )rcnMm+wnhdl)esakn )\")ryPiam)O)v} no)zdOui   =(  \""
    "    '+'= ~(=(= (\n= ()=(@_& ~@'                   (        = '%;"
    " go. PO  'x )p)g                   <h  )kh    )      \"k\"  M ai"
    ")  <eu  +    =z q   =x    =eowe  =v+kO= c=  !    =dMe ij lmp    "
    "     c     p      _) c_O    |tp=^ c sz^o ^sibnj^jis <^q^e aQ ^ t"
    "hug^eiawyq=!=a<]= vhyh >      '@  =\n   '= &=,-\n=,=\n (&  [ ()*"
    "        ,  *  ,*= )& (\n, \n  , \n   ,& ,*('l'c )    l \\  's 'l"
    "' ci lcad eilm nq_dkv<=-^      (@,       \n(&  !\n=, (  , (/\n @"
    " m       h =*(.c. (&'  ( =,(=(|  (|(|(|\n&(,&  [g(, (,\n tni^-,="
    "(=(u t&(=)(~,=d(,( = ,sep wpg\n@\nan,\n&,*  n  a,$=,=|\n.(      "
    " c'=~,~,=(,\n=(,[(o ad#( \n    (d x=~,\n,='e,  ( =   '~),~=(,   "
    "     O=\n  ( oc -~= ,\n =<, <=,(@='\\i|~ i~, =\n =i   o       i="
    "-\n,i(=v t   l t l      *   ,+ *+     *, p u=\n!=,\n(=,(   e    "
    "   a*=, \n=\n[f(< n~('n i   n,n \n,eu s(  ,|&^,(=l op='&|,\n ,+\n"
    "^\n !,(= o, =/-=(/=%+ , e, (= ,()=               |     ]   |OtvO"
    ")\n    )      \n)k   ')n\n'                   -   (=( | - ( >   "
    "   - =(";

FILE *source_file;

/* Undo the unary run-length encoding in place.  Walking from right to left,
   every space becomes a copy of the (already decoded) character after it. */
void run_length_decode(void) {
  int i;
  for (i = PAYLOAD_LENGTH - 2; i >= 0; i--) {
    if (payload[i] == ' ')
      payload[i] = payload[i + 1];
    else if (payload[i] == '@')
      payload[i] = ' ';
    else if (payload[i] == '$')
      payload[i] = '\0';
  }
}

/* Inverse Burrows-Wheeler transform of payload[] (the last column of the
   sorted rotation matrix) into decoded[], using the LF-mapping. */
void inverse_bwt(void) {
  int i, c, row, total = 0;

  /* earlier_count[i] = number of times payload[i] occurred before i */
  for (i = 0; i < PAYLOAD_LENGTH; i++)
    earlier_count[i] = symbol_count[(unsigned char)payload[i]]++;

  /* first_position[c] = number of characters smaller than c */
  for (c = 0; c < ALPHABET_SIZE; c++) {
    first_position[c] = total;
    total += symbol_count[c];
  }

  /* Rebuild the text from its last character to its first */
  row = BWT_PRIMARY_INDEX;
  for (i = PAYLOAD_LENGTH - 1; i >= 0; i--) {
    decoded[i] = payload[row];
    row = earlier_count[row] + first_position[(unsigned char)payload[row]];
  }
}

int main(int argc, char **argv) {
  int i;

  run_length_decode();
  inverse_bwt();

  /* Write the extracted source:  fprintf(f, "%.*s", 2064, source)  */
  source_file = fopen(decoded + FILE_NAME_OFFSET, "w");
  fprintf(source_file, decoded + FORMAT_OFFSET, SOURCE_LENGTH,
          decoded + SOURCE_OFFSET);
  fclose(source_file);

  /* Build "gcc -O3 ag.c -o ag;./ag" followed by the program's arguments.
     The command buffer is also a source of sprintf (undefined behaviour).
     With current glibc the buffer ends up holding only " " and the last
     argument, so the shell runs the last argument as a command. */
  sprintf(payload, decoded + COMMAND_OFFSET);
  for (i = 1; i < argc; i++)
    sprintf(payload, "%s %s", payload, argv[i]);

  exit(system(payload));
}
