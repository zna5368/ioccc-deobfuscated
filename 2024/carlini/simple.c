#include<stdlib.h>
#include<stdio.h>
#include<fcntl.h>

/*
 * Intel 4004 (plus 4001/4002/4003 support chips) simulated at the logic gate
 * level, wired up to the Busicom 141-PF keyboard and print drum.
 *
 *   1. An arithmetic-coded, LZ-compressed integer stream is unpacked.
 *   2. That stream is a "circuit program" (gates, repeats and sub-circuit
 *      calls) which is expanded into ~158,000 logic gates.
 *   3. The program ROM (argv[1]) and the micro-op table are written into
 *      placeholder gates of the circuit.
 *   4. The circuit is simulated forever, only re-evaluating gates whose
 *      inputs changed, while a "trap" port in the circuit is used to print
 *      lines on the drum printer and feed key presses read from stdin.
 */

#define BIG (1 << 24)              /* size of every large working array */

/* gate operations (each gate uses 4 ints: op, input1, input2, input3) */
enum { WIRE, NOT, OR, AND, XOR, MUX };

/* other circuit-program opcodes; anything >= END returns from decode() */
enum { CALL = 6, REPEAT = 7, END = 9 };

/* an input reference this large names a sub-circuit argument, not a gate */
#define ARGUMENT_BASE 200000

/* the compressed text is a base-1968 number: 123 "visible" characters x 16 */
#define RADIX 1968

/* bytes of dependents storage for each gate (holds 17500 gate numbers) */
#define DEPENDENTS_BYTES 70000

/* layout of the circuit's trap / I/O port inside the gate value array */
#define TRAP_ACTIVE    0    /* circuit wants the outside world to act    */
#define TRAP_MINUS     1    /* line being printed is marked with a '-'   */
#define TRAP_IS_WRITE  2    /* port write (1) or port read (0)           */
#define TRAP_IS_RAM    3    /* RAM port (1) or ROM/TEST port (0)         */
#define TRAP_PRINT     4    /* the write fires the print hammers         */
#define TRAP_KEY_INPUT 5    /* 8 gates where the C code puts a key code  */

#define DRUM_COLUMNS 18     /* digits printed per line                   */
#define KEY_INTERVAL 128    /* TEST port reads between keyboard changes  */
#define KEY_TABLE    325    /* key table offset in charset (index -1=EOF)*/

/* arithmetic-coded, LZ-compressed circuit description */
char *compressed_circuit =
    "8 {}	o;	({ #} U{t *	;1}	&x;_;	k}rI}6{Y}O{{ p{}	O}}A Z}	x o{ >} W{ } V{	q{	.}t;	n{{ P{U{m;; >; }{&;{	{  e{	g}	}{ w	o { R};_{1;6}N}7{	(V{6} G;E}3} k{Y}T}4}Y;	C{ {[5;S;	L	J{	 s;,} {@{ [{ c;5; J{ m;	D}g{*{	^;	ec;	w;T;D;	{W}1;I; a}	{@*{y}l .}D{:	a}{	&	v}	] }w	b{	};	x{	z}	g}H}#;%;v2{ n} [{&;	n;U}a} ;	 ;	H}Yc F{; m	=}W;	V};i{}f}c;H}	{l{ p;	b}_;v} H{	`{{		r;w}3{X;C;O} O} B}z} {	b{ d;V{8  	},	}9}	d{*{	a{ Jb{t{${	;6{}@}-	e{q{W}}B{	e;	; }0{ -;x}}Q; } x{ L~}	J}	Sn}{/	G;X;	y{9}UY} 7}#d{e;l;	?{}	^{M}{	S} K;	+{ .{r{ z}Vu;} O{[;8{}g}	C{ U;a{={$ m}{}_}	{ 8}	o} Z{ u;=;c;	;Q;G; H^}X; M{ 6{[ '{	/;1}	g;		 E} ;[{	v{?;c; }; -P 3{ V	} @} >;}52}c{ YF}	^[;z;	R{a} `{V~} =}	r	3}p}{[g;)}	R -;4}	g{{	z}^;}V; b	1}n}	F;	R;{s;l}{c;	E${u}o} e; @}	,} ';S;	=	:[{ D}4};}} 1;2{	^{A{w !}	n}2} 5{8}z{ &;o;	I}	u}{	]; A}s;	S;9 {	3;T;p;9;=}d	G , M}iw{	}X} 8;;}{ }Ew}	{	=l	?{	u ,} ;k{	1}N;	]}	3{1{{>}c;	Q}M i{ I{.{	o}L{	?@}@{2; Y;O -> K}	.{V; X}D;	< E}	; 5{ /{ ;;a{	&	;P{}{p{Z{}	X }+;	q; 9|{z;y	7} g{G;I{	O{ ;	^}(}	_:} }h G	A{;{-{ ~}}	a{ u{	)}	7{Y{ L}; /;	m[;b{W	|; ]} &	h;;	} &}_} ;Y}	3	8{*;	S;}	O{0w;0{ :; `{	D{3{	y{	ae{	N; *{t} 	}X}aB});{	x{ #;{	};	J	; 8} }{2;	.; }	E;W};{&{{	b{Y;	^;		;	h	: +} C{}j}=;	d{	Q}N;;N{	)4 ={	q{f}D;p} 2}%	K} }LY{ b{	W RK{  F;	w1f{ J{	|;n}	E};	;	 t; O;L;{.9} <;	B{F	;	0{ w; b{&}l;	{ 9{Q{ }	K}; E;; {9	W;C}_{	9 o;	o{p} % Z;}	QZ;	6	Q}	I{	{p}s} }	} _}	b{	{v{	q{X;[{%}	J		 )E;	;V} P}	t{ {{p	P	;	B}R{E.0{ !}h;7 x} 8}`;C}s{x	;D;x;	%{ w};	s$ ; y *;j{}{{{{8X;	G{4w; t e{) ?}{{x;5;	{*}}/}F 5}| s}E;&} j w{s C{	}	n{ t}Z;	{2;+}s	^} {	_; {} *{k}	S; , v2{	q;	5 }a}m{ }0; 	{ }!};	N} ;8K	{	W}W;	qO{C{	o{e}+	p} =; ;<;W{; V	pP} %;	n}	;{}$J} } V{s;	v{ B; ,8; j;		1{]{; <;	I} 	Q}	; q} 3{ /{7{H}{ r{ A}{;);	O{	3{	2	L{d 6{J{ Z;5; {	V{;{q;	B{u}+D; @ 5 s %	{	>;}	z{	q}	#}	W}d}}}	9{		`}+{C},;I	za{m; -}E}}]N{d} #} U} .{}	{ N;7{w	G	b; }} k } E{	({L{} D{	|{)}>G;}{ {t}=}	f}	;X}) b;>}h} ;;;} ?(;=; G;m	u0; }U;	A;u} Y} ; d}{2;p;	V.}	E};	;^},} {	{;F}4} 	; Y; z;>}	<{O;y{ K{  ;){	n{ 2}q	^l.;	5F	 ;6;; K} (;t;a{ C	Z}T;	p{	yt{K{g;5} ; _{   ; ={}N{;  f{} S;^;!{C;{ x{	{k{V{/{ *{	D{$}	U	#{	}	`{fa(;R%{	#} 9 e} S<;	A};=;K	-{ a;@{5 KW	 0;T}{(} C;	:q{n;{{${ ,}	}A{ '{	_i;	}	4;	; ~} q})}>	~;/;0;O;}	F}A D{;/{G{	@;	U} _{	Z [;	N{E;~	z;`{ {Z}{ Y{r $}	0G}	m    ";

/* --- arithmetic decoder state --- */
long range = 1;                     /* current width of the coding interval */
int fraction;                       /* position of the code inside it       */
int bit_history[1000][2];           /* how often each context gave 0 / 1    */

/* --- circuit construction --- */
int stream[BIG];                    /* LZ-decompressed circuit program      */
int stream_length;
int call_buffer[BIG];               /* fresh copy of stream for each CALL   */
int function_offset[9];             /* where each sub-circuit starts        */
int call_args[BIG];                 /* gate numbers passed to a sub-circuit */
int gates[BIG];                     /* 4 ints per gate: op, in1, in2, in3   */
int *gate_out;                      /* where decode() writes the next gate  */
int next_constant;                  /* next placeholder gate to fill in     */

/* --- simulation --- */
int gate_count;
char value[BIG];                    /* current output of every gate         */
int *dependents[BIG];               /* gates that read each gate's output   */
int dependent_count[BIG];
unsigned long *clean_mask;          /* 1 bit = gate (block) may be skipped  */
int active;                         /* which half of clean_mask is current  */

/* --- I/O --- */
char *trap;                         /* the trap port inside value[]         */
int print_offset;                   /* print hammer flip-flops in value[]   */
int *charset;                       /* drum characters, then key table      */
int print_minus;
int test_reads;                     /* ROM/TEST port reads (JCN) so far     */
int last_key_change;
int key_released;

/*
 * Decode one bit with an adaptive binary arithmetic decoder.  Each context
 * remembers how many 0s and 1s it produced so far and uses that as the
 * probability of the next bit.  The compressed text is read as base-1968
 * digits: a first character worth 0..122 (skipping \n, \r, " and \) times
 * 16, then a cheap low part made only of whitespace, or one of { } ; plus
 * whitespace, which the IOCCC size tool does not count.
 */
int get_bit(int context)
{
    if (range < RADIX) {
        int c;

        range *= RADIX;
        fraction *= RADIX;

        c = *compressed_circuit++;
        fraction += (c - 1 - (c > 10) - (c > 13) - (c > 34) - (c > 92)) << 4;

        c = *compressed_circuit++;
        if (c < 33)
            fraction += (c ^ 8) * 2 % 5;
        else
            fraction += (c ^ 6) % 3 * 4 + (*compressed_circuit++ ^ 8) * 2 % 5 + 4;
    }

    int *count = bit_history[context];
    int split = range * (count[0] + 1) / (count[0] + count[1] + 2);
    int bit = fraction >= split;

    if (bit) {
        fraction -= split;
        range -= split;
    } else {
        range = split;
    }
    count[bit]++;
    return bit;
}

/*
 * Read a variable-length (Elias-gamma like) number: a unary count of extra
 * bits followed by the bits themselves.  Small numbers are cheap.
 */
int read_number(int bits, int context)
{
    int offset = 1 << bits;
    int result = 1;
    int i;

    context *= 99;
    while (!get_bit(++bits + context))
        ;
    bits--;
    for (i = 0; i < bits; i++)
        result = result * 2 | get_bit(context);
    return result - offset;
}

/* LZ decompress the integer stream: literals and back-references */
void lz_decompress(void)
{
    int *out = stream;
    int count = read_number(9, 0);

    while (count--) {
        if (get_bit(1)) {
            int *source = out - read_number(4, 2) - 1;
            int length = read_number(1, 3) + 1;
            while (length--)
                *out++ = *source++;
        } else {
            int sign = 1 - 2 * get_bit(8);
            *out++ = sign * read_number(3, 9);
        }
    }
    stream_length = out - stream;
}

/*
 * Expand a circuit program into gates.  Gate inputs are stored relative to
 * the gate being built (or as a sub-circuit argument).  REPEAT re-runs the
 * previous instruction after adding constants to some of its fields, by
 * rewriting the instruction stream in place; that is why a CALL first
 * copies the whole stream before running the sub-circuit.
 */
int *decode(int *code)
{
    int op;
    int *previous = code;

    while ((op = *code++) < END) {
        int gate = (gate_out - gates) / 4;
        int *start = code - 1;
        int i;

        if (op < CALL) {
            int inputs = 1 + (op > NOT) + (op > XOR);

            gate_out[0] = op;
            for (i = 0; i < inputs; i++) {
                int ref = *code++;
                int source = ref < ARGUMENT_BASE ? gate - ref
                                                 : call_args[ref - ARGUMENT_BASE];
                gate_out[i + 1] = source;
                if (source)
                    dependents[source][dependent_count[source]++] = gate;
            }
            gate_out += 4;
        } else if (op != REPEAT) {
            int function = *code++;
            int args = *code++;

            for (i = 0; i < args; i++)
                call_args[i] = gate += *code++;
            for (i = 0; i < stream_length; i++)
                call_buffer[i] = stream[i];
            decode(call_buffer + function_offset[function]);
        } else {
            int remaining = --*code++;
            int changes = *code++;

            if (remaining) {
                int field = 0, increment = 0;
                for (i = 0; i < changes; i++) {
                    increment += code[changes];
                    previous[field += *code++] += increment;
                }
                code = previous;
            } else {
                code += changes * 2;
            }
        }
        previous = start;
    }
    return code;
}

/* write bit planes of a table into the first input of placeholder gates */
void place_constants(int *table, int bits, int log_size)
{
    int bit, i;

    for (bit = 0; bit < bits; bit++)
        for (i = 0; i < 1 << log_size; i++)
            gates[next_constant++ * 4 + 1] = table[i] >> bit & 1;
}

/* one full sweep over the circuit, skipping gates whose inputs are unchanged */
void simulate_step(void)
{
    int next = active ^ gate_count;
    int gate = 0;

    while (gate < gate_count) {
        int level, i;

        /* walk the 3-level mask (blocks of 4096, 64 and 1 gates) */
        for (level = 2; level >= 0; level--) {
            int block_size = 1 << 6 * level;
            int block = gate / block_size;
            unsigned long bits =
                clean_mask[active + gate_count / 64 * level + block / 64] >> block % 64;
            int skipped = 0;

            while (bits & 1) {
                gate = gate - gate % block_size + block_size;
                bits /= 2;
                skipped = 1;
            }
            if (skipped)
                break;
        }

        int *g = gates + gate * 4;
        int a = value[g[1]], b = value[g[2]], c = value[g[3]];
        int old = value[gate];

        switch (g[0]) {
        case WIRE: value[gate] = a;     break;
        case NOT:  value[gate] = a ^ 1; break;
        case OR:   value[gate] = a | b; break;
        case AND:  value[gate] = a & b; break;
        case XOR:  value[gate] = a ^ b; break;
        default:   value[gate] = a ? b : c; break;
        }

        if (old != value[gate]) {
            /* mark readers dirty: later gates now, earlier ones next sweep */
            int n = dependent_count[gate];
            while (n--) {
                int reader = dependents[gate][n];
                unsigned long *mask = clean_mask + (reader <= gate ? next : active);

                for (i = 0; i < 3; i++)
                    mask[gate_count / 64 * i + (reader >> 6 * (i + 1))] &=
                        ~(1UL << (reader >> 6 * i) % 64);
            }
        }
        gate++;
    }

    /* the finished half becomes "all clean" and the other half is used next */
    for (int i = 0; i < gate_count / 16; i++)
        clean_mask[active + i] = -1L;
    active = next;
}

/* print one line from the 18 print hammer columns */
void print_line(void)
{
    int column;

    putchar(print_minus ? '-' : ' ');
    for (column = 0; column < DRUM_COLUMNS; column++) {
        /* the hammer registers are not stored in printing order */
        int reg = (column + 3) % DRUM_COLUMNS - (column > 15);
        char *h = value + print_offset + reg * 21;

        /*
         * 4 bit gates read together as a little-endian int; that number
         * mod 18 indexes this column's permuted drum characters.
         */
        int position = (h[0] | h[1] << 8 | h[2] << 16 | h[3] << 24) % 18;
        putchar(charset[column * DRUM_COLUMNS + position]);
    }
    print_minus = 0;
    puts("");
}

/* react to the circuit's trap port after a sweep */
void handle_io(void)
{
    if (trap[TRAP_ACTIVE]) {
        int is_write = trap[TRAP_IS_WRITE];
        int is_rom = !trap[TRAP_IS_RAM];

        print_minus |= trap[TRAP_MINUS];
        test_reads += is_rom & !is_write;
        if (is_write & is_rom & trap[TRAP_PRINT])
            print_line();
    }

    /*
     * alternate between "no key" and the next stdin key every 128 reads
     * (test_reads only grows in a trap, so this always follows a trap)
     */
    if (test_reads - last_key_change > KEY_INTERVAL) {
        int key, i;

        last_key_change = test_reads;
        key_released = !key_released;
        key = key_released ? 0 : charset[KEY_TABLE + getchar()];
        for (i = 0; i < 8; i++)
            trap[TRAP_KEY_INPUT + i] = key >> i & 1;
    }
}

int main(int argc, char **argv)
{
    int i, j;
    int rom[4096], column[256];

    if (argc < 2)
        exit(42);

    /* getchar() returns EOF instead of waiting when no key is typed */
    fcntl(0, F_SETFL, O_NONBLOCK);

    lz_decompress();

    /* header of the decompressed stream */
    int *in = stream;
    gate_count = *in++;
    trap = value + *in++;
    print_offset = *in++;
    for (i = 0; i < gate_count; i++)
        dependents[i] = malloc(DEPENDENTS_BYTES);
    int functions = *in++;
    for (i = 0; i < functions; i++)
        function_offset[i] = *in++;
    int main_circuit = *in++;

    /* build the gates */
    gate_out = gates;
    in = decode(stream + main_circuit);

    /* program ROM: 4096 bytes, stored as 8 bit planes of placeholder gates */
    next_constant = *in++;
    FILE *rom_file = fopen(argv[1], "r");
    for (i = 0; i < 4096; i++)
        rom[i] = fgetc(rom_file);
    place_constants(rom, 8, 12);

    /* micro-op table: 256 instructions x 16 steps of 6-bit micro-ops */
    next_constant = *in++;
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 256; j++)
            column[j] = in[j * 16 + i];
        place_constants(column, 6, 8);
    }
    charset = in + 4096;

    /* two halves (current / next sweep) of three mask levels; 0 = dirty */
    clean_mask = calloc(gate_count, 16);

    while (1) {
        simulate_step();
        handle_io();
    }
}
