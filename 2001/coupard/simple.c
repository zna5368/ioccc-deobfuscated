#include <unistd.h>
#include <time.h>

/*
 * Huffman compressed speech data. Every pair of printable characters holds
 * 13 bits as a base 91 number whose digits start at '#'. Once decompressed
 * the data holds the word table followed by the phoneme sound samples.
 */
unsigned char *compressed_data =
    "KZs2ITTwhwZYvec@JbYxOjf9-TZRGDb/el7#q(`SZ#|_neTwq\\MqJ5cVgteK-ReK-(Mq8"
    "+D'6Ui0tG88vXJ-Tu{VI=d%cR]h7CumwBq\\-#{thj8fw$OEfEvLHP13_##w.OD[7Cw2]<"
    "T{|[F}L;L:*+A#PwLnp{9'M3Mr|w_|unm'}$*(5]_$?O9zO{{wz4p6vP8Ipu}$BQospf=-"
    "Isnyl'|g53o^c`ov-P`-x+|ZAd<e?<'b9P|LkZOf{B-8`K([srqv&gy1,:}$|s7D{yN6M#"
    "cQyKpC|*_|#xA#'YfQ}$k$kr7dM#dcnWDg|PHdA#^j&q}$x@(a;k2JB]50ZKp{xRwbkTm."
    "v\\a3fJ@V[`J#fN`s|sZeHmmHX7`JKiry4sm,bUfz69{rt'k*pBq^l,ut6UVvb9N%r%l:P"
    "y3r[.Z/pBkaz2J4u{GTu)hyp#%gbS$r`mzi9G|3M,r}-Dt?w)_##%fQt$k&n]e$HzFXu(|"
    "Fvt`i#$A#|ykrz+nhs'P#|pm*9[_#,5P#}$}$x&s(i,HzO'&d&g?/_$P#kZ7Dn]e$J{zi8"
    "ypi|hq$_.|cLx`sy;f8GRMQM#7R5$hym*:^`sxrvtfQ|z_N|ye[7dH[}#Ts\\59V@|#z}$"
    "}$y;usL)vXFiA0AukZ8H|fyKurd]|s5<|xq@Cpm,}${,e$KufO?/2Mt$zmM#vtfQ|zRKlE"
    "A#P&fPOwJ#pf_$?/2#@|$w}$?fLxA#^-}$cM^aA#&g#wIv?^|yY#s)Lwt'K#ONyoXKYkpC"
    "l0m}D[9OGU7n([ZrL)[8eRy7uR*PJ#^5xRG>zSf>p]ZrJ_[9q?qgYe^4$r3lY$4SdsSyNv"
    "J_l&w2I?q#fN*F1n|s9OGUv|w&l+pR-3Nudtyn@|Dfq#^o|s9Pt=oZCol/{VdgkLdwyn1y"
    "Ded}?Dzi}#Kje#d}Y${8pSt<q?qgIv|`DH?9}$O{k(LgE;|KU-tJnWOw*F@YP#|yY$b4|."
    "50[2d}D'kaqhp;JE$r&fH&'6nykYGCv>K$t?yLbwtUn[F{ncOx_.:K#nwF#f|HQs&<&bfQ"
    "PE?:uJm*&\\S=*/@|We*w@$@f0,*H])WL<i@^xX@)(a.>MV*p{Zqf&zH%#############"
    "######u2#A#_$A#_$A#_$A#_$@tA#QI#3#0&:p#?1[P*6WQ@i#C1{[;@|1}S%^r#g.d^z#"
    "M}$ziZe|kU6oZbq5$kF]4[E^nA#_.|}e$Ku^r&WA#_$?/*P_$@t&gA#[;_$A#^j2#A#_*d"
    "}@v^z#8?D?D9Q`s}$5$L|u<3JC/xVz;#s^{?9$M}$x_Y,7uP$";

/* Decompressed word table followed by the 8 kHz unsigned 8-bit samples */
unsigned char speech[6789];

#define TOTAL_BITS 8476
#define BITS_PER_PAIR 13
#define DIGIT_BASE 91
#define FIRST_DIGIT '#'

/*
 * Hard coded Huffman tree. Node n has its 0 branch at n and its 1 branch at
 * n + 1. A branch value (character - 59) that is zero or more is a leaf symbol
 * and a negative value is minus the index of the next node.
 */
#define LEAF_OFFSET 59
const char huffman_tree[] = "C9B7351A@D-/E+F?')G>H%J#=I";

/*
 * The first 145 symbols form the word table. Each word is a list of phoneme
 * numbers ending with END_OF_WORD. The remaining symbols are 4-bit samples at
 * 4 kHz that are scaled to 8 bits and doubled to play at 8 kHz.
 */
#define WORD_TABLE_SIZE 145
#define END_OF_WORD 9

/*
 * Phonemes (pronounced loosely):
 *   0 "ah"  1 "eh"  2 "ee"  3 "n"  4 "oh"  5 "r"  6 "s"  7 "t"  8 "v"
 * Phoneme n is played from speech[phoneme_start[n]] up to
 * speech[phoneme_start[n + 1]].
 */
int phoneme_start[] = {145,  1145, 1745, 2545, 3045,
                       4045, 4345, 5145, 5745, 6369};

/* Words in the word table */
enum {
    WORD_ZERO = 0, /* zero .. twelve are words 0 .. 12 */
    WORD_TWENTY = 13, /* twenty, thirty .. ninety are words 13 .. 20 */
    WORD_THIRTY = 14,
    WORD_THE_TIME_IS = 21,
    WORD_HOURS = 22,
    WORD_MINUTES = 23,
    WORD_SECONDS = 24,
    WORD_AND = 25,
    WORD_TEEN = 26 /* an "n" that turns thirty .. ninety into thirteen .. */
};

/* Silence written between words */
#define SILENCE_SAMPLE 'w'
#define PAUSE_WORD 145
#define PAUSE_UNIT 1145
#define PAUSE_NUMBER 1745
#define PAUSE_INTRO 3045

/* Read one bit of the compressed data, most significant bit first */
int read_bit(int bit) {
    int pair = bit / BITS_PER_PAIR;
    int value = (compressed_data[pair * 2] - FIRST_DIGIT) * DIGIT_BASE +
                (compressed_data[pair * 2 + 1] - FIRST_DIGIT);

    return (value >> (BITS_PER_PAIR - 1 - bit % BITS_PER_PAIR)) & 1;
}

/* Decode the compressed data into the word table and phoneme samples */
void decompress(void) {
    int bit, node = 0, out = 0;

    for (bit = 0; bit < TOTAL_BITS; bit++) {
        int symbol = huffman_tree[node + read_bit(bit)] - LEAF_OFFSET;

        if (symbol < 0) {
            node = -symbol;
            continue;
        }
        node = 0;

        if (out < WORD_TABLE_SIZE) {
            speech[out++] = symbol - 2;
        } else {
            speech[out++] = symbol << 4;
            speech[out++] = symbol << 4;
        }
    }
}

/* Write a run of silence samples */
void pause_for(int samples) {
    static const unsigned char silence = SILENCE_SAMPLE;
    int i;

    for (i = 0; i < samples; i++)
        write(1, &silence, 1);
}

/* Play every phoneme of a word from the word table */
void say_word(int word) {
    int pos = 0;

    /* skip over the earlier words */
    while (word > 0)
        if (speech[pos++] == END_OF_WORD)
            word--;

    for (; speech[pos] != END_OF_WORD; pos++) {
        int phoneme = speech[pos];
        write(1, speech + phoneme_start[phoneme],
              phoneme_start[phoneme + 1] - phoneme_start[phoneme]);
    }
}

void say_number(int n) {
    if (n < 13)
        say_word(WORD_ZERO + n);
    else if (n < 20)
        say_word(WORD_THIRTY + n - 13);
    else
        say_word(WORD_TWENTY + n / 10 - 2);
    pause_for(PAUSE_WORD);

    if (n > 12 && n < 20)
        say_word(WORD_TEEN);
    else if (n > 20 && n % 10 != 0)
        say_word(WORD_ZERO + n % 10);
    pause_for(PAUSE_NUMBER);
}

int main() {
    time_t now = time(&now);
    struct tm *t;

    decompress();

    say_word(WORD_THE_TIME_IS);
    pause_for(PAUSE_INTRO);

    t = localtime(&now);
    say_number(t->tm_hour);
    say_word(WORD_HOURS);
    pause_for(PAUSE_UNIT);

    say_number(t->tm_min);
    say_word(WORD_MINUTES);
    pause_for(PAUSE_UNIT);

    say_word(WORD_AND);
    pause_for(PAUSE_WORD);

    say_number(t->tm_sec);
    say_word(WORD_SECONDS);
    pause_for(PAUSE_WORD);

    return 0;
}
