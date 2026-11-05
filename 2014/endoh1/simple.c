#include<stdio.h>

/*
 * This program prints the source of a second program, main.c. That program
 * holds a small virtual machine, and the bytecode it runs draws a QR code. To
 * print its own source it must contain a string with all of its data, so this
 * program has to rebuild that string exactly.
 *
 * The string is made of three parts, stored here one after the other:
 *
 *     qr_bytecode          the program run by the virtual machine
 *     version_adjustments  a small table read by that bytecode
 *     packed_source        the source of main.c in compressed form
 *
 * Format of packed_source
 * -----------------------
 * The text is a list of tokens that ends at the terminating nul:
 *
 *     token < BACK_REFERENCE_TOKEN
 *         A run of (token - LITERAL_RUN_TOKEN) characters follows and is copied
 *         to the output. SOURCE_STRING_MARK in a run stands for the whole
 *         string above, written as a C string literal.
 *
 *     token >= BACK_REFERENCE_TOKEN
 *         Copy (token - BACK_REFERENCE_TOKEN) characters from earlier in the
 *         output. The next character is the distance back to them, plus
 *         DISTANCE_BIAS. The copy can overlap the characters it is writing.
 *
 * Format of qr_bytecode
 * ---------------------
 * Every instruction is a printable character, so the program fits in a string:
 *
 *     '#' to '7'   push register 0 to 20
 *     '8' to 'L'   write the top of the stack to register 0 to 20
 *     'M' to 'U'   add, subtract, multiply, xor, less than, divide, modulo,
 *                  shift right, equal
 *     ']' '_' 'a'  if, else, end if
 *     'b' '^' '`'  while, do, end while
 *     'c' 'd'      load from memory, store to memory
 *     'e' 'f'      print a character, exit
 *     'g' 'h'      increment a register, decrement a register
 *     'j' to '~'   push a number written in base 21
 *
 * The readable version of the bytecode is in src.rb, and the readable version
 * of the virtual machine is in vm.c.
 */

#define SOURCE_CAPACITY 5000

#define LITERAL_RUN_TOKEN 34
#define BACK_REFERENCE_TOKEN 92
#define DISTANCE_BIAS 36
#define SOURCE_STRING_MARK 'Q'

/* The packed text has a placeholder where this line's newline belongs. */
#define INCLUDE_LINE "#include<stdio.h>"
#define INCLUDE_LINE_LENGTH (sizeof(INCLUDE_LINE) - 1)

static const char qr_bytecode[] =
    /* Set the base addresses of the data areas in memory. */
    "nsu{AntynCnuq}Bnu{sEotln>"

    /* Count the characters of the text to encode. */
    "b)+c^g+@`"

    /* No text was given: unpack this program's own source to use as the
     * text, which is what makes the QR code a quine. */
    "+]_osk{;j@bkg&c<'^or'Q]bh'lvQ^kg&c:%n|N]_optj9lwg+)d:bkg$c8#^#g+)d8`a%"
    "g+)d8`_g&;bh'oqQ^)g+&kcNlyMc+)d8`a`g+@u)|d8a"

    /* Try each QR version from 1 to 40 in turn. */
    "k=bl}(Q^"

    /* Total number of codewords in this version. */
    "og(O{MK6lML(rRpOpM866OsRlmN(qQ]##OsR#M_(lQoOaN9"

    /* Number of error correction codewords, using version_adjustments. */
    "$mvOwwRor~}(cNmkM:"

    /* Degree of the Reed-Solomon generator polynomial. */
    "q(Q]%_(uU]}_{8b%mRh#S^`#mOaaD"

    /* Number of blocks and the size of each block. */
    "%/RI4$4SNH$%N4RlMG/2MJ24O3NF(tQ?"

    /* Stop at the first version that has enough room for the text. */
    "1lN*N+Q]"

    /* Exponent and logarithm tables for the Galois field GF(2^8). */
    "lmq9l8b$^#h$#.d,dxv#mSOPmR8`"

    /* Coefficients of the Reed-Solomon generator polynomial. */
    "/lM;bh&^/lM:k8b%&Q^-h%c9.#,/&N$McPc%-d8,$c8``"

    /* Draw the three finder patterns in the corners. */
    "7:b%^h%:79b$^%$7ON8r%Qrh$QOn%qN%O~M$qN$OMpRmPQO%rQ$rQMkQN77O#dj#Nkd$7O"
    "%d8``"

    /* Draw the alignment patterns. */
    "(rRmM:(luN](%mRMmO%lNRmO_loa8%%O<b(lQh'lQO^ln;b'%N&O^6sN#M9slltwzh&TmS"
    "$'%R#ON$oNS&pRM7O$'%S#ON$oNS&pSMd9``"

    /* BCH error correction code for the version information. */
    "(tpkO9$:nk8b#^%}kx#O%lMtpk#OQOP:#mR8`"

    /* Draw the timing lines and the version information. */
    "}(qQO<b'7M{N^kh'Q]7sN'M8q;k'N_7vN'nSM8'nR;$%M'TamS&7O#d#7O&d8`"

    /* Draw the format information: level L, mask 4. */
    "z9b$^h$rQ]6yN$_$$pQaM8m{px$TmSt6O#NsM$nRmUd#7Osd77sNOsd8`"

    /* Turn the text into a byte mode bit stream with padding bytes. */
    "j=nk;|8k9b1g(Q^)$c:%]k*Q]g$9%_h*]+wnS_+sTaa:&%{RM<%{S{O_&<#wkP8#a;'(0d"
    ":`"

    /* Calculate the Reed-Solomon error correction for every block. */
    "184>b)^15)OM:23h)QN<'/M=b(^'h(Q]0h#c_kah%0d9`b'/O(Q^(/SlM90%M(/RM;&$c&"
    "kckQ,-$c.&kccMcOP$&d9g(=``"

    /* Place the interleaved bits along the zigzag path, applying the mask. */
    "6:69j=b5g(Q^3(2lNUO<b4'Q^s;b&^%%pQM8k$7O#cQ]0(2(Q]'2O3'Q]'_3aN_1'5OMaM"
    "ch&T$mR#nRMPmSkU$7O#d8_a%%mS]l_%mRmS]$]h$9j_la_$6N]g$9j_laaaN:`g'<``"

    /* Print the modules as ANSI colored spaces with a quiet zone. */
    "7oM:6%N9b%g$Q^6%N8b%g#Q^lpeoqemtemw#jQ7#QO$jQO7$QOk#$7OMQ]k_#$7OckQaOr"
    "ONepneluelue`lpeoqempepneue`"

    /* Exit with 0 after printing, or with 1 if the text never fit. */
    "kf_a`lf";

/*
 * One character for each QR version from 1 to 40. The bytecode uses these to
 * correct its formula for the number of error correction codewords, because
 * the QR standard's table is irregular.
 */
static const char version_adjustments[] =
    "))*+,**,*)-)/),0).0(6/2+667,(&$###;=@D*0";

/*
 * The source of the generated program, in the format described at the top of
 * this file. It unpacks to the code of vm.c with the bytecode string replaced
 * by SOURCE_STRING_MARK. It cannot be stored unpacked, because the generated
 * program needs these exact bytes to print its own source.
 */
static const char packed_source[] =
    ";#include<stdio.h>@int H[9`%.],*s,*c,d,t;`;'main(`-9n,char*v[]){for(s="
    "c=H+3`XI;(*++s=Q[d++]););for(;n>1&&(*++s=v[1][tgANs=H;d=*c++%93,d-9;){"
    "int v=*s,g[]={n+v,v-n,n*_,#^_,,<v,n?v/n:0`,#%`,,v>>n,v==n}ay>t=0;d<4&&"
    "d>=2*!!n&&(c-=d/3*2_KK3+*c++,t||v!=98+d);)t+=v++/6-16?0:v/2%3-1cq*d-14"
    ";t>0_t$<3_z(105<*c_X'=t*21aq@-106;n=d>76?s--,g[d-77]:d>55?H_1)21]=n,*_"
    "@_8(9?*++s_4&12>d_C`f3]+=21-d*2:d<34?t:_Zadat57<d?putchar(n),v:6<a](g+"
    "99]=a{d2#:`xa6,n;}return _.";

static char source[SOURCE_CAPACITY];
static char *source_end = source;

static void append_text(const char *text)
{
    while (*text) {
        *source_end++ = *text++;
    }
}

/* Write the three parts of the data as one C string literal. */
static void append_source_string(void)
{
    *source_end++ = '"';
    append_text(qr_bytecode);
    append_text(version_adjustments);
    append_text(packed_source);
    *source_end++ = '"';
}

/* Copy a run of characters and return the position after the run. */
static const char *unpack_literal_run(const char *packed, int length)
{
    while (length-- > 0) {
        char character = *packed++;

        if (character == SOURCE_STRING_MARK) {
            append_source_string();
        } else {
            *source_end++ = character;
        }
    }
    return packed;
}

/* Must go forward one character at a time so an overlapping copy repeats. */
static void unpack_back_reference(int length, int distance)
{
    while (length-- > 0) {
        *source_end = source_end[-distance];
        source_end++;
    }
}

static void unpack_source(void)
{
    const char *packed = packed_source;
    int token;

    while ((token = *packed++) != 0) {
        if (token < BACK_REFERENCE_TOKEN) {
            packed = unpack_literal_run(packed, token - LITERAL_RUN_TOKEN);
        } else {
            int distance = *packed++ - DISTANCE_BIAS;

            unpack_back_reference(token - BACK_REFERENCE_TOKEN, distance);
        }
    }
    source[INCLUDE_LINE_LENGTH] = '\n';
}

int main(void)
{
    unpack_source();
    puts(source);
    return 0;
}
