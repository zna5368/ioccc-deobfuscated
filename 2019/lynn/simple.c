#define x 0/**/
/*
 * The line above must stay the very first line of this file.
 *
 * The compiler's output is meant to be glued in front of this source:
 *
 *     (./prog < fib.hs; cat prog.c) > fib.c
 *
 * That output looks like
 *
 *     #define x \
 *     346,344,0,0,...,16,342,/*
 *
 * so x becomes the list of numbers that make up the compiled heap image, and
 * the trailing comment opener swallows the "#define x 0" above (C comments do
 * not nest, so the comment ends at the "* /" right after the 0).  When this file
 * is compiled on its own, x is just 0 and the heap starts out empty.
 */
#include <stdio.h>

typedef unsigned Cell;

enum { HEAP_SIZE = 1 << 26 };

/*
 * Combinator numbers.  Any heap value below FIRST_NODE names a combinator,
 * anything at or above it is the index of an application node, which is a
 * pair of cells: heap[n] is the function and heap[n + 1] is the argument.
 */
enum {
    HALT    = 0,  /* reduction stops when this reaches the top of the stack */
    INT     = 1,  /* INT n f     -> f (INT n)       an integer literal       */
    S       = 2,  /* S f g x     -> f x (g x)                                */
    C       = 3,  /* C f x y     -> f y x                                    */
    R       = 4,  /* R x f y     -> f y x                                    */
    B       = 5,  /* B f g x     -> f (g x)                                  */
    T       = 6,  /* T x f       -> f x                                      */
    GETCHAR = 7,  /* GETCHAR _   -> lazy list of the rest of stdin           */
    K       = 8,  /* K x y       -> x               also True and Nil        */
    I       = 9,  /* I x         -> x               (K I) is False           */
    LE      = 10, /* LE (INT a) (INT b) -> a <= b                            */
    MUL     = 11,
    ADD     = 12,
    SUB     = 13,
    DIV     = 14,
    PUTCHAR = 15, /* PUTCHAR (INT n) rest: print n, continue with rest       */
    Y       = 16, /* Y f         -> f (Y f)         fixpoint                 */
    CONS    = 17, /* CONS h t n c -> c h t          Scott-encoded list cell  */
    /*
     * 18..23 are the strict versions of LE..PUTCHAR:
     *     op a b -> b (a raw_op)
     * Both arguments are integers, and an integer applied to f gives
     * f applied to the integer, so this evaluates b, then a, and finally
     * calls raw_op = op - 8 with both values in place.
     */
    STRICT_LE      = 18,
    STRICT_PUTCHAR = 23,
    FIRST_NODE     = 24
};

/*
 * The heap.  When compiled behind the compiler's output, x fills it with the
 * compiled program: heap[0] is the size of the image and heap[1] is the node
 * holding main.
 */
Cell heap[HEAP_SIZE] = { x };
Cell heap_top = FIRST_NODE;              /* next free cell */
Cell *definitions = heap + HEAP_SIZE / 2; /* decoded top-level terms */
Cell *stack = heap + HEAP_SIZE - 3;      /* spine stack, grows downwards */
Cell image_size;                         /* nonzero: running a compiled program */
Cell head;                               /* combinator currently being reduced */

/*
 * The compiler itself is stored as a combinator term, Huffman coded into a
 * bit stream that is written in base 85 (characters ' ' to 't').
 */
char *packed_bits =
    "33Yb59@&iBFApt;[[h3V19\\3<:4cJ!U 2eT18pC,Qqik4J:sh?HUXMrR(-l0R\""
    "!eKZcI!@E(@B,C/*!aogn5LbK/e=2CmReb+6,]kD!iOC9DEOC9Dc1EV6976c?&s)"
    "Be;P;E^tl2eUYkg*#Yf:6^d[Mg_P;VGCr823^L_<X+j2,%nD20Ls lmpi&I(*hV="
    "+p aTO`r.b1<i[/R\\t1,KBt)\\%;\\@27H>^#d6B!tb-on27d%i_OS5(W5eV-=M"
    "14kYO);Fe7k!N41<iX*T,kHW,&)_l&L)'0:Jj%j7^h+JU /9pZn&Td:&T%'TE<7>"
    "LW%m/R\\rON3-=G]^akjT778YCJ7B8e-5E#RX R=Ig8#/pDdAI;=a[ ISbp't+ZL"
    "J;lUO71C)b5[Y)qTWmFJ)G1ehmS<.`n3RnE IG+G_A`CE=?hZU)bScgt7R3GNs+V"
    "(HQLL_R)n4;]#cUR.p>5!^4T3pQg^o//WLATCE18mSUme[Q<53e:')Q_%<L$1lKO"
    "nFD(R3%*jj85VW+#8Wt*Ud,1D7AKcdh<9r%igC$2</HD7X$K_0Rr/>L.*D2%;[0B"
    "+#8UANT1.tSd/^@L$&a6^g@jYNMC7O<rPWO5AfA;C'9WnLn9E:0d:R\\hAZ^m=/0"
    "9d.R$Jd%:Gp hB-g4&N!VO)$;iED1:F`^`UrnWCSZ!L$Tdma!_hn7H0F$^MT(-Ln"
    "9F_Ljfp9*h8Er!<.g.h_7@b0l03?a+]`=dIoY>WCKOk&#[9FCQ#WFoga(tK<[PG6"
    "@5k2KY\\ oW':M;e//kd\"[l,_V?UlZ,m(Hh?O81mhM!G18 ]7m?X7e^[7EZYj[;"
    "kR,YXD\\X,!k6.HF8Z(V^^nBYea^NIL:]lG0(/\\Ik<m`>jPam;F-A,(tVN+bG@<"
    "L'J 1D8dE*hN87:TsccSKVE7KP+k8^5qZIkJG_fH_$MS--5(!*St/1:b+g8(/*";

/*
 * Everything that is not a Huffman coded bit lives in this second string:
 *
 *  - the 11 Huffman symbols (a symbol with code c is stored as the character
 *    c + 32), ended by a tab;
 *  - the 6 digit characters of the mixed radix numbers, followed by an
 *    unused space;
 *  - the stream of numbers and string literals read by the decoder.  What is
 *    left of it once the term is decoded is the "#define x \" header that the
 *    compiler prints in front of its output.
 */
char *side_data =
    " $6I7H#v Jw\t"   /* Huffman symbols */
    "{\v}\f;\t"       /* mixed radix digits 0..5 */
    " "
    "\v }\f }\f \f \f \v }\f \v\t } }\t ;\f }\f \f\t {\f\v } \f\t }\f"
    " ;\t \f\f \t\t {\f {\v\v \t \v\v\v \f }\v\v } }\v\v \f\v\v \f\v"
    "\v ;\v\v \v ;\v\v \t\v\v }\v\v \v\t ;\v\v \t\v\v }\v\v }\v\v ;\f"
    " }\t }\t }\v\v ;\f }\t \f\f \v\t \f\f \t\v\v }\v\v ;\v\v \t\v\v "
    "}\v\v {\t \f\f \f\v\v \t\v\v }\t ;\f }\f }\v\v \t\v\v ; \f\f ;\f"
    " }\f \t\f }\f ;\v\v \f\v\v {\t }\f \f\v\v \v {\f\v }\v\v ;\v\v }"
    "\f {\f\v ;\t {\f\v \f\v\v ;\t {\f\v \f\t }\f \v ;\v\v ;\v\v ;\t"
    "\v {\v\v {\f\v }\f\v }\v\v \t\f\v ;\v\v \t\v\v ;\v\v {\f\v \t\v"
    "\v } \f\v\v \f\f\v {\v} \f\v\v }\v\v \v\v} }\f \f\f {\t\v ;\f \f"
    "\f \v {\t \f\f \t\f ;\f } {\t \t\f } }\f \v\f} }\t \f \f \f\f ;"
    "\v} ;\t \f\t }\f }\t }\t\v \f\v} }\f} }\f \v\f} \f\t }\t {\f}  :"
    "!#$%&*+./<=>?@\\^|-~\t\t\t \v\v\v \f\t   \n\t\v\f} {\v\v {\v\v "
    "\f\f }\f  --\t\t\t \f\t ;\f} ;\f \v\t ;\f }\f \v\t }\f \f\t \v\f"
    "\v ;\v\v {\f\v {\f\v \v\f\v } }\v\v \f\v\v }\f {\f\v \v\f} \t\v"
    "\v ;\v\v \t\t }\v\t \t\v\v \v\v\v  \\'\"\t\t\f} {\f\v ;\f \t\t }"
    "\v{\f \v\v\v }\t} }\v\v \v\v\v \t\f} ;\v\f \v\f} }\f} \f\f\v \f"
    "\v\f }\f} }\f  :\t\f\v} \v\t \v\v\v }\f }\t }\f \v\v\t \f\v\t }"
    "\f\v \v\f\v {\f\t \f\t{\f \v\f\v ;\f\f \v\v\t \f\v\f \t\v\t  of"
    "\t\v\v\v ;\t\v \f\t\v } \f\f {\t\v }\t\v }\t\v \f\f \f\t }\f\v "
    "\f\v} {\f\v \t\v\f \f\f }\f \f\v\v ;\t\v \t\v\v \v\t {\f\t {\f\t"
    " }\f }\t ;\v} \t\v} {\v} ;\f }\f \f\f }\t\f \t\v\f }\v} \f\v} \f"
    "\v\f \f\f \v\v\v ;\t }\f} \t\v\v \t\f\v {\t  ->\t\v\v\v \f\t{\f "
    "\t\t{\f \t\v\f \v\f\v \v\t\v ;\t \f\v} ;\t \f\f} }\f} \f\v; \t\f"
    "} \v\f\v \t\f\f }\t \f\f} }\f} \v\t \t\v} \t\t {\f} \t\f\v \v\f"
    "\f \f\f} \v\v; }\f\f \t\v} }\f} }\f} }\f} {\f\v }\v} \f\v} \f\v}"
    " ;\f\v \f\v} \v\f\v {\f\v }\v} ;\t\v ;\t\v {\f} ;\f\v ;\t\v \f\f"
    "} ;\f\f \f\f} }\v; {\t} {\f \t ;\f ;  \v\t\v \t\v} \f\f\v }\f} }"
    "\f} }\f} }\f} ;\f} \f\f} \f\v; }\f\f {\t\f {\v} ;\f  let\t;\f  i"
    "n\t\t\f }\f ;\f} \f\f} \v\v; }\f\f } {\v} ;\f  case\t;\f }\v\v "
    "\t\f \v\f} \v\t ;\f} \f\f} }\t {\f} {\v\v \v\f} {\t ;\f} \f\f} "
    "\f\v; {\t\f {\v} {\t\v }\v\t {\t \f\f ;\t }\f} }\f} \f\f} }\f\v "
    "\t\t \t\f} ;\f} \f\f} ;\f} \f\f} }\f\f }\f\f \t\t} \f\f\f \t\t "
    "\v\t \t\v;  \f\v; \t\v; \t \v } {\f; {\f; {\f; } \t } } \f\v; } "
    "}\f\f \v \f\v;  } \v {\f ;\v\v  {\f; } } \v\f; {\f; }\f; } }\f; "
    "\v }\f; \v \f\f }\f; \f\f \v\f; }\v; ;\v; \f\f }\f; ;\f \t ;\t }"
    "\f; } }\f ;\f} \f\v\v  : <= * + - /\t}\f; {\f\v }\f \t\f} {\v} "
    "\v\t} \v\f; }\f\f \f\t\v {\t} \t\f} ;\f\v }\t  data\t\v\f\f \v\t"
    " \v\t} \v\v; }\f; ;\t} {\v\v {\f; \v\t\f {\t\f \v\t\f }\f\f \v\v"
    "; ;\v; \f\t\v }\t} \v\t} }\f; \v\v\v \v\t} ;\f } ;\f} {\t} {\t} "
    "}\v\v \v\t }\f\v \v\t ;\v\v ;\t{\f \v\t} } {\t \v }\t} \v\t} {\f"
    "; \t\f ;\f} }\t} \v\t} {\t\f {\t\v \f\v\v \t\f \t\v\f ;\f; #defi"
    "ne x \\";

char *huffman_symbols;
char *digit_chars;
char *cursor;            /* read position in the number/string stream */

Cell bit_word;           /* 32 bits decoded from 5 base 85 characters */
int bits_left;           /* how many bits of bit_word are still unread */
int input_char;          /* last character read by GETCHAR */

/* Allocate an application node (function argument). */
Cell apply(Cell function, Cell argument)
{
    heap[heap_top++] = function;
    heap[heap_top++] = argument;
    return heap_top - 2;
}

/*
 * Read a number written with the characters of digit_chars.  The digits
 * alternate between base 6 (digit used as is) and base 3 (digit / 2), which
 * lets the author pick the cheapest characters for the size tool.  Any
 * character that is not one of the 6 digits ends the number and is skipped.
 */
Cell read_number(void)
{
    Cell value = 0, weight = 1, divisor = 1, digit;

    for (;;) {
        /* the search always succeeds: the cursor itself points into the string */
        digit = 0;
        while (*cursor != digit_chars[digit])
            digit++;
        cursor++;
        if (digit >= 6)
            return value;
        value += digit / divisor * weight;
        weight = weight * 6 / divisor;
        divisor = 3 - divisor;
    }
}

/* Read a tab terminated string literal and build it as a list of INTs. */
Cell read_string(void)
{
    Cell ch = *cursor++;
    Cell character, rest;

    if (ch == '\t')
        return K;                        /* Nil */
    character = apply(CONS, apply(INT, ch));
    rest = read_string();
    return apply(character, rest);
}

int read_bit(void)
{
    int k;

    if (bits_left-- == 0) {
        bit_word = 0;
        for (k = 0; k < 5; k++)
            bit_word = 85 * bit_word + *packed_bits++ - 32;
        bits_left = 31;
    }
    return (bit_word >> bits_left & 1) != 0;
}

/* Index of the Huffman symbol with this code, or -1 for an unfinished code. */
int find_symbol(Cell code)
{
    int index;

    for (index = 0; huffman_symbols[index] != '\t'; index++)
        if ((Cell)huffman_symbols[index] - 32 == code)
            return index;
    return -1;
}

/*
 * Decode one term.  Symbols:
 *   0       application: two more terms follow
 *   1       a number n follows:  0 -> string literal,
 *                                1..7 -> combinator Y, CONS, 18..22,
 *                                8.. -> the definition n - 7 places back
 *                                       (a reference past the first one
 *                                       finds 0 and ends the program)
 *   2       a number follows: integer literal
 *   3..10   combinators S, C, R, B, T, GETCHAR, K, I
 */
Cell decode_term(void)
{
    Cell code = 0, number, function;
    int symbol;

    do
        code = code * 2 + read_bit();
    while ((symbol = find_symbol(code)) < 0);

    switch (symbol) {
    case 0:
        function = decode_term();
        return apply(function, decode_term());
    case 1:
        number = read_number();
        if (number == 0)
            return read_string();
        if (number < 8)
            return number + 15;
        return *(definitions - (number - 7));
    case 2:
        return apply(INT, read_number());
    default:
        return symbol - 1;
    }
}

/* Argument of the n-th node on the spine stack. */
Cell argument(int n)
{
    return heap[stack[n] + 1];
}

/* Value of an evaluated integer argument (INT value). */
Cell int_argument(int n)
{
    return heap[argument(n) + 1];
}

/*
 * Replace the node that holds the last of the arity arguments with the
 * application (function argument) and pop the consumed spine.
 */
void rewrite(int arity, Cell function, Cell arg)
{
    Cell *node = heap + stack[arity];

    node[0] = function;
    node[1] = arg;
    stack += arity;
}

void reduce_strict(void)
{
    rewrite(2, argument(2), apply(argument(1), head - 8));
}

void reduce_int(void)
{
    rewrite(2, argument(2), stack[1]);
}

void reduce_s(void)
{
    Cell f = apply(argument(1), argument(3));
    rewrite(3, f, apply(argument(2), argument(3)));
}

void reduce_c(void)
{
    Cell f = apply(argument(1), argument(3));
    rewrite(3, f, argument(2));
}

void reduce_r(void)
{
    Cell f = apply(argument(2), argument(3));
    rewrite(3, f, argument(1));
}

void reduce_b(void)
{
    rewrite(3, argument(1), apply(argument(2), argument(3)));
}

void reduce_t(void)
{
    rewrite(2, argument(2), argument(1));
}

void reduce_getchar(void)
{
    Cell list = K;

    input_char = getchar();
    if (input_char >= 0)
        list = apply(apply(CONS, apply(INT, input_char)), apply(GETCHAR, 0));
    rewrite(1, I, list);
}

void reduce_k(void)
{
    rewrite(2, I, argument(1));
}

void reduce_i(void)
{
    rewrite(2, argument(1), argument(2));
}

void reduce_le(void)
{
    rewrite(2, I, int_argument(1) > int_argument(2) ? apply(K, I) : K);
}

void reduce_mul(void)
{
    rewrite(2, INT, int_argument(1) * int_argument(2));
}

void reduce_add(void)
{
    rewrite(2, INT, int_argument(1) + int_argument(2));
}

void reduce_sub(void)
{
    rewrite(2, INT, int_argument(1) - int_argument(2));
}

void reduce_div(void)
{
    rewrite(2, INT, int_argument(1) / int_argument(2));
}

/*
 * Print one element of the output list.  A compiled program prints
 * characters, the compiler prints the heap image as comma separated numbers.
 * The rest of the list is then asked for again: Nil case = the (unused)
 * printf result, Cons case = (STRICT_PUTCHAR I).
 */
void reduce_putchar(void)
{
    Cell printed = printf(image_size ? "%c" : "%u,", int_argument(1));
    rewrite(2, apply(argument(2), printed), apply(STRICT_PUTCHAR, I));
}

void reduce_y(void)
{
    rewrite(1, argument(1), stack[1]);
}

void reduce_cons(void)
{
    Cell f = apply(argument(4), argument(1));
    rewrite(4, f, argument(2));
}

void (*reduce[])(void) = {
    reduce_strict, reduce_int, reduce_s, reduce_c, reduce_r, reduce_b,
    reduce_t, reduce_getchar, reduce_k, reduce_i, reduce_le, reduce_mul,
    reduce_add, reduce_sub, reduce_div, reduce_putchar, reduce_y, reduce_cons
};

int main()
{
    Cell main_term;

    huffman_symbols = side_data;
    digit_chars = side_data + 12;
    image_size = heap[0];

    if (image_size) {
        /* compiled program: the heap already holds it, main is heap[1] */
        definitions = heap + 2;
        heap_top = image_size;
    } else {
        /* the compiler: unpack it, then print the "#define x \" header */
        cursor = digit_chars + 7;
        while ((*definitions = decode_term()))
            definitions++;
        puts(cursor);
    }

    /* main (GETCHAR 0) with Nil case 0 and Cons case (STRICT_PUTCHAR I) */
    main_term = *--definitions;
    *stack = apply(apply(apply(main_term, apply(GETCHAR, 0)), 0),
                   apply(STRICT_PUTCHAR, I));

    /*
     * Lazy graph reduction: walk down the left spine until a combinator is
     * on top, then rewrite.  The run ends when the bottom-of-stack sentinel
     * (a 0 below the root) surfaces.
     */
    while ((head = *stack) != HALT) {
        if (head < FIRST_NODE)
            reduce[head < STRICT_LE ? head : 0]();
        else
            *--stack = heap[head];
    }

    /* the compiler ends its output with a comment opener (see the top) */
    return image_size || puts(packed_bits);
}
