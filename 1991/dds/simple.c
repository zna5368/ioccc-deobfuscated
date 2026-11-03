#include <stdio.h>

/*
 * The BASIC compiler is not written in C.  It is written for a tiny
 * virtual machine called COGNIMP$ and this C file is only the interpreter
 * for that machine.
 *
 * Machine state: four text registers 0-3 (each a NUL terminated string).
 * The "I" instruction compares the first character of register 0.
 *
 * Instructions (one opcode letter followed by its operands):
 *
 *   C r n    Copy:       read exactly n characters of input into register r
 *   O r      Output:     write register r to the output file
 *   G x      Goto:       jump to label $x
 *   N r      iNcrement:  add one to the first character of register r
 *   I x      If:         jump to label $x if register 0 starts with x
 *   M r d    Match:      read input into register r up to delimiter d
 *                        (at end of input the C file is finished & compiled)
 *   P text`  Print:      write the literal text up to the backquote
 *   $ x      label x     (no operation)
 */

#define NUM_REGISTERS 4
#define REGISTER_SIZE 99

#define OUTPUT_FILE "a.c"
#define COMPILE_COMMAND                                                        \
  "cc -Wno-error -Wno-implicit-function-declaration -Wno-implicit-int "        \
  "-include stdio.h a.c"

/*
 * The compiler, written in COGNIMP$.
 *
 * The generated C program keeps the current BASIC line number in l and is
 * one big switch statement with a case per BASIC line.  r[] is the GOSUB
 * return stack (p is its pointer), f[] and m[] hold the loop start line and
 * loop limit of each FOR variable and b[] is the INPUT buffer.
 */
static char program[] =
    /* global variable declarations of the generated program */
    "Pint r[200],f[200],m[200],b[99],p,l`"
    /* count register 0 up from NUL to 'A' */
    "$l"
    "N0"
    "IA"
    "Gl"
    /* declare the BASIC variables ,A,B,...,Z */
    "$A"
    "P,`"
    "O0"
    "IZ"
    "N0"
    "GA"
    "$Z"
    /* start of main: a switch on the current line number l */
    "P;main(){for(;;)switch(l){`"
    "Pcase 9999:return 1;`" /* END jumps to line 9999 */
    "Pdefault:`"            /* lines that do not exist: try the next one */
    "P;l++`"

    /* $B: end of a statement that changes l */
    "$B"
    "P;break`"

    /* $a: end of a statement, then start a new BASIC line */
    "$a"
    "P;`"
    "M1 "                   /* line number into register 1 */
    "P\ncase `O1P:l=`O1P;`" /* case NUMBER:l=NUMBER; */

    /* $ : skip spaces and dispatch on the first letter of the keyword */
    "$ "
    "C01"
    "I "
    "IF" /* FOR */
    "IG" /* GOTO, GOSUB */
    "IN" /* NEXT */
    "IP" /* PRINT */
    "IE" /* END */
    "IR" /* REM, RETURN */
    "II" /* IF, INPUT */
    "IL" /* LET */
    "GR" /* anything else is treated like REM / RETURN */

    /* $G: GOTO or GOSUB, the third letter tells which */
    "$G"
    "C01"
    "C01"
    "M2 "
    "M3\n" /* target expression into register 3 */
    "IT"
    "IS"
    /* $T: GOTO exp */
    "$T"
    "Pl=`O3"
    "GB"
    /* $S: GOSUB exp, push the next line onto the return stack */
    "$S"
    "Pr[p++]=l+1;l=`O3"
    "GB"

    /* $R: REM or RETURN, the second letter is always E so read the third */
    "$R"
    "C01"
    "M0\n"
    "IM"
    "Pl=r[--p]`" /* RETURN: pop the return stack */
    "GB"
    "$M" /* REM: generate nothing */
    "Ga"

    /* $F: FOR var = exp TO exp */
    "$F"
    "M0 "
    "C21" /* the loop variable into register 2 */
    "O2"
    /* $Q: copy "= exp" up to the TO keyword, a lone T is just copied */
    "$Q"
    "M0T"
    "O0"
    "C01"
    "IO"
    "PT`O0"
    "GQ"
    /* $O: remember where the loop starts and where it ends */
    "$O"
    "M3\n" /* limit expression into register 3 */
    "P;f['`O2P']=`O1P+1;m['`O2P']=`O3"
    "Ga"

    /* $N: NEXT or INPUT (reached from $I), the third letter tells which */
    "$N"
    "C01"
    "C01"
    "M1 "
    "M2\n" /* the variable into register 2 */
    "IU"
    /* NEXT var */
    "O2P++;if(`O2P<=m['`O2P'])l=f['`O2P'];else l++`"
    "GB"

    /* $P: PRINT "string" or PRINT exp */
    "$P"
    "M0 "
    "M0\n"
    "I\""
    "Pprintf(\"%d\\n\",`O0P)`"
    "Ga"
    "$\""
    "Pputs(`O0P)`"
    "Ga"

    /* $E: END */
    "$E"
    "M0\n"
    "Pl=9999`"
    "GB"

    /* $U: INPUT var */
    "$U"
    "O2P=atoi(fgets(b,99,stdin))`"
    "Ga"

    /* $I: IF exp THEN exp, or INPUT when the second letter is N */
    "$I"
    "C01"
    "IN"
    "M0 "
    "Pif(`"
    /* $K: copy the condition up to THEN, a lone T is just copied */
    "$K"
    "M0T"
    "O0"
    "C01"
    "IH"
    "PT`O0"
    "GK"
    /* $H: the jump to the THEN line */
    "$H"
    "P)l=`"
    "M1 "
    "M1\n"
    "O1P;else l++`"
    "GB"

    /* $L: LET var = exp */
    "$L"
    "M0 "
    "M1="
    "O1P=`"
    "M1\n"
    "O1"
    "Ga";

int ch;
char registers[NUM_REGISTERS][REGISTER_SIZE];

/* Find "$label" in the program and return the position just after it */
static char *find_label(int label) {
  char *pc = program;
  while (pc[0] != '$' || pc[1] != label)
    pc++;
  return pc + 2;
}

int main(int argc, char **argv) {
  char *pc, *reg;
  int count;
  FILE *basic = fopen(argv[1], "r"), *c_file = fopen(OUTPUT_FILE, "w");

  if (!basic || !c_file)
    return 1;

  pc = program;
  for (;;) {
    switch (*pc++) {
    case 'M': /* Match */
      reg = registers[*pc++ - '0'];
      while ((ch = fgetc(basic)) != EOF && ch != *pc)
        *reg++ = ch;
      if (ch == EOF) {
        /* end of the BASIC program: close the switch and main, compile */
        fputs("}}\n", c_file);
        fclose(c_file);
        return system(COMPILE_COMMAND);
      }
      *reg = 0;
      pc++;
      break;
    case 'P': /* Print */
      while (*pc != '`')
        fputc(*pc++, c_file);
      pc++;
      break;
    case 'O': /* Output */
      reg = registers[*pc++ - '0'];
      while (*reg)
        fputc(*reg++, c_file);
      break;
    case 'C': /* Copy */
      reg = registers[*pc++ - '0'];
      for (count = 0; count < *pc - '0'; count++)
        *reg++ = fgetc(basic);
      *reg = 0;
      pc++;
      break;
    case 'I': /* If */
      if (registers[0][0] == *pc)
        pc = find_label(*pc);
      else
        pc++;
      break;
    case 'G': /* Goto */
      pc = find_label(*pc);
      break;
    case 'N': /* iNcrement */
      registers[*pc++ - '0'][0]++;
      break;
    default: /* '$' label: skip the label name */
      pc++;
      break;
    }
  }
}
