#include <math.h>

/*
 * The compressed image.  It is a sequence of base 1968 digits (1968 = 123 * 16)
 * that feed the arithmetic decoder.  Each digit is written as:
 *
 *   - one "high" character worth 0..122: any character from 1 to 127 except
 *     the four that cannot appear raw in a string ('\n', '\r', '"', '\\'),
 *   - a "low" part worth 0..15, written as either
 *       one whitespace character               (worth 0..3), or
 *       one of '}' ';' '{' plus one whitespace (worth 4..15).
 *
 * The string contains raw tab, vertical tab and form feed characters.
 */
static const char *data =
"	{k/;	y{ q ; }	c {	@;	={ 	S}	c}	W;;	{4}	k |; w{	+9;{;	8; 9{	S;	/}	y{ K}	{;}	l{	{ ~{ ;	V}k}g< t{	E	v;M{ B}y}	<{7;/;	Y} t}kp; Y} $Ha{e} w};} R} /{>}a	;} ;	`	$W-}	D}B; e;f;*;	~;A;s O{	o;>{1; m{ `} R}]{ T} v}={ I} ; }a?&; A}$;W;R{u} `; j}W;s{e}	A;[	R;	X  P; 4 ,F;({<8{#;%}@J{)}	}o^*{u/{'}]{	*}	}	;{ r}	f	/;}e} }w{ ${{;,; @ d	$}];>(}	I{ d}	&;	U}	{	y;Y}	{ P{	R} T}_{ }R } l	{ T}"
"';	|; ${=}	H} (}}8{cp{ s} #}+}	3}kF}<H	 .{ }G}x;	r	D c{; W; {	b;6; k{}B;*};	]} ~	{ ;;} !}}	x}v}n;^;	6V}Y{ h; ~	%*}! H; G{ r{ f;Y{ i}z} N  %}.{;	( 	v} _}	h; 7;<}	^;Z;0; ;	<;<; M; N{	}	_{O} !{f{]{M{;A{}	0;S}${	@;x}y}@	L;1	t{ 3{c{s{_{	`{	D{ ]}!;	${	_J;v+ }	3{B; ]{	}	E6	.x{?+; {x; }v{$};6}T; O; ; (}X7}	j; @} :}#	c{ !{ }x	KXt} >; ?{ c; ;	W;	; l;} h}p}	i{ %	}P}	/{	*}	%L; ;	!{ S{ n} "
"x;  { 1	J;v{	U}({	@ X{ k} H;4;e J	6;;v; G{{]	&{A d{ lM{;K;;	4-{}} p h{;	{	rW;	v{;	f}	}1{^&{9{{ ;~;n;q{	9 R	6{	{ u;a;	;	U;	;Y}	+}}2sk; 8	{	JK;'i;	;$;	W{	P!{{{P	} [;	(;Q; Un;+}g{C;{{	; <{	vS} b;6`} ?{+	%;	}n;q{ r}k; ;{c{ S} 2}~{	4;RW v} R;	kI}|; d; [ O}5; ;;}Z d	{ {&;h	o{ V	v ;	_{{/}  F{f{r{4{{?{ 4;S}	:;];E}	;	&} #e !{>{H; {O{ 0;} H;	p; w}>{1}{	-} 4;"
"S}}	u L{ y} %;2  |{(}	/;,{ )}Y;g}	G}v;T}	};}i {{};[{ E{q} g;T{ ={}R;	k{ j;_;h}gPc;({	F;6}	}} 3	,}<; 0	 P;{'t}u};		}U}s{8{ E} >{}E	{G{H :{  Yog}	}F  D{ R{	 -;M?;= q}_ U	{ ;	 I	{ |{{}	 	1{,}{ x{{ U{ s;J}}	6{>7;,{ D{	{{ ;]}	;M; &}{ V}	n{&	T~;({	}[;	r{#	u{X 9;L; Uf})}   {T}		p{	N;	>{	>	}}D} m{1{	{}X; o}	w}$}	^v} K  f	,}	^3; { @{_} _{	o;	4}	h}H;#.{	{}	;	<{ {G{ $;{ "
"z {a{{D;	?|}{{ ;	`} }	Q}j;4} 	3{Q}	{	* ;}r{a}	} R{p @;  N{ {f; A;8}L	$}{ }}J{ }	k{r} { [; -;p{	I{ {	&}J;	T}	?{Z{>;	5>; ];  wz ^}	u;);	H}	; L	&;	V	E{1{g;C} V} ~;U; ^{	J; { /}	{;(}y} aK /}	.};K;N{w{ `{	}T{l`; #;N{lX;	?; +}{ 	w{	;	q;	z;_;y} 8} 	&{X}	V{ WG}	,; [}U{	v{	Q;	w{	[	Y}N	Yu i{ {!A{}{ b0;	X~} ;-; 8{	E }	;F{	y{}{	";

#define EMBEDDED_BASE 1968    /* digit base of the built-in string */
#define STDIN_BASE    256     /* digit base when reading bytes from stdin */

#define MAX_PIXELS    (1 << 20)   /* up to a 1024x1024 image */
#define NUM_CONTEXTS  83

/* Adaptive probability contexts used by the arithmetic decoder */
#define CTX_SPLIT        0    /* 0..2: split a 8x8/16x16/32x32 block? */
#define CTX_SIGN         3    /* sign of a coefficient */
#define CTX_GOLOMB_BITS  4    /* mantissa bits of every number */
#define CTX_HEADER       5    /* header fields (shared with luma runs) */
#define CTX_RUN          5    /* zero run length: +10 for chroma */
#define CTX_LEVEL        25   /* coefficient magnitude: +10 chroma, +20 low freq */
#define CTX_END_OF_BLOCK 65   /* 65..72: by block size and chroma */
#define CTX_MODE         73   /* prediction mode of a block */

#define MODE_DC          0    /* other modes are directional predictors */

int planes[3][MAX_PIXELS];          /* Y, Cg and Co pixel planes */
int coeffs[2048];                   /* DCT coefficients + transform scratch */
int contexts[NUM_CONTEXTS][2];      /* count of 0 bits and 1 bits seen */
int code;                           /* arithmetic decoder value */
int range = 1;                      /* arithmetic decoder range */
int width;                          /* image width (and row stride) */
int base;                           /* digit base of the input */
int luma_quant, chroma_quant;       /* quantizer step sizes */

/* Value 0..3 of a whitespace character in the low part of a digit */
static int whitespace_value(int c)
{
    switch (c) {
    case ' ':  return 0;
    case '\v': return 1;
    case '\t': return 2;
    default:   return 3;    /* '\f' */
    }
}

/* Value 0..2 of the punctuation that starts a two character low part */
static int punctuation_value(int c)
{
    switch (c) {
    case '}': return 0;
    case ';': return 1;
    default:  return 2;     /* '{' */
    }
}

/* Shift one more input digit into the arithmetic decoder */
static void refill(void)
{
    int c, high, low;

    range *= base;
    code *= base;
    if (base == EMBEDDED_BASE) {
        c = *data;
        if (c) {    /* past the end of the string the digits are zero */
            high = c - 1 - (c > '\n') - (c > '\r') - (c > '"') - (c > '\\');
            c = data[1];
            data += 2;
            if (c <= ' ')
                low = whitespace_value(c);
            else
                low = 4 + punctuation_value(c) * 4 + whitespace_value(*data++);
            code += high * 16 + low;
        }
    } else {
        c = getchar();
        code += c < 0 ? 0 : c;
    }
}

/* Decode one bit with an adaptive probability taken from a context */
static int decode_bit(int context)
{
    int *count = contexts[context];
    int total, split, bit;

    if (range < base)
        refill();
    total = count[0] + count[1] + 2;
    split = range * (count[0] + 1) / total;
    bit = code >= split;
    if (bit) {
        code -= split;
        range -= split;
    } else {
        range = split;
    }
    count[bit]++;
    if (total > 63) {   /* forget old statistics */
        count[0] /= 2;
        count[1] /= 2;
    }
    return bit;
}

/*
 * Decode a non negative number in Exp-Golomb form: a unary count of
 * mantissa bits (one context per position) followed by the mantissa bits.
 */
static int decode_number(int context)
{
    int num_bits = 0, value = 1;

    while (!decode_bit(context + num_bits))
        num_bits++;
    while (num_bits--)
        value = value << 1 | decode_bit(CTX_GOLOMB_BITS);
    return value - 1;
}

/* Fixed point (x1024) DCT-II basis function value */
static long dct_basis(int n, int x, int freq)
{
    return lrint(cos(acos(-1) / n * (x + 0.5) * freq) * sqrt(2 - !freq) * 1024);
}

/*
 * One dimensional inverse DCT of size n applied to n lines.  Element k of
 * line l is at in[k * in_step + l * line_step] and the result goes to
 * out[k * out_step + l * line_step], rounded and scaled down by 2^shift.
 */
static void inverse_dct(int *out, int out_step, int *in, int in_step,
                        int line_step, int n, int shift)
{
    int line, x, freq, sum;

    for (line = 0; line < n; line++) {
        for (x = 0; x < n; x++) {
            sum = 0;
            for (freq = 0; freq < n; freq++)
                sum += in[freq * in_step + line * line_step] * dct_basis(n, x, freq);
            out[x * out_step + line * line_step] = sum + (1 << (shift - 1)) >> shift;
        }
    }
}

/* Read the quantized DCT coefficients of one plane of a block */
static void decode_coefficients(int log_size, int is_chroma)
{
    int area = 1 << (log_size * 2);
    int pos, sign, magnitude;

    for (pos = 0; pos < area; pos++)
        coeffs[pos] = 0;
    for (pos = 0; pos < area; pos++) {
        if (decode_bit(CTX_END_OF_BLOCK + (log_size - 2) * 2 + is_chroma))
            break;
        pos += decode_number(CTX_RUN + is_chroma * 10);    /* skip zeros */
        sign = 1 - 2 * decode_bit(CTX_SIGN);
        magnitude = decode_number(CTX_LEVEL + (is_chroma + (pos < area / 8) * 2) * 10) + 1;
        coeffs[pos] = sign * magnitude * (is_chroma ? chroma_quant : luma_quant);
    }
}

/* DC prediction: average of the row above and the column to the left */
static void predict_dc(int *pixels, int x, int y, int size)
{
    int a, sum = 0;

    for (a = 0; a < size; a++) {
        if (y)
            sum += pixels[-width + a];
        if (x)
            sum += pixels[a * width - 1];
    }
    coeffs[0] += x && y ? sum / 2 : sum;
}

/*
 * Directional prediction: every pixel is interpolated (in 1/8 pixel steps)
 * from the row above or the column to the left along a slope.
 */
static void predict_directional(int *pixels, int size, int mode)
{
    int horizontal = mode < 17;
    int slope = horizontal ? 9 - mode : mode - 25;
    int a, b, i, pos, frac, other_edge, ref[2];

    for (a = 0; a < size; a++) {
        for (b = 0; b < size; b++) {
            for (i = 0; i < 2; i++) {
                pos = (a + 1) * slope;
                frac = pos & 7;
                pos = (pos >> 3) + b + i;
                other_edge = pos < 0;
                if (other_edge)     /* project onto the other edge */
                    pos = (pos * 8 + slope / 2) / slope - 2;
                if (pos > size - 1)
                    pos = size - 1;
                ref[i] = other_edge ^ horizontal ? pixels[pos * width - 1]
                                                 : pixels[-width + pos];
            }
            pixels[horizontal ? b * width + a : a * width + b] +=
                ref[0] * (8 - frac) + ref[1] * frac + 4 >> 3;
        }
    }
}

/* Decode the square block of 2^log_size pixels at (x, y), splitting it up */
static void decode_block(int x, int y, int log_size)
{
    int size, area, mode, plane, quarter, half;
    int *pixels;

    if (log_size > 5 || (log_size > 2 && decode_bit(CTX_SPLIT + log_size - 3))) {
        half = 1 << (log_size - 1);
        for (quarter = 0; quarter < 4; quarter++)
            decode_block(x + quarter % 2 * half, y + quarter / 2 * half, log_size - 1);
        return;
    }

    size = 1 << log_size;
    area = size * size;
    mode = decode_number(CTX_MODE);
    for (plane = 0; plane < 3; plane++) {
        pixels = planes[plane] + y * width + x;
        decode_coefficients(log_size, plane > 0);
        if (mode == MODE_DC)
            predict_dc(pixels, x, y, size);
        /* rows into the scratch area, then columns into the image */
        inverse_dct(coeffs + area, 1, coeffs, 1, size, size, 10);
        inverse_dct(pixels, width, coeffs + area, size, 1, size, 10 + log_size);
        if (mode != MODE_DC)
            predict_directional(pixels, size, mode);
    }
}

static void put_clamped(int value)
{
    putchar(value < 0 ? 0 : value > 255 ? 255 : value);
}

int main(int argc, char **argv)
{
    int a, luma, cg, co, tmp, log_width, height;

    /* any argument means: read the compressed image from stdin */
    base = argc > 1 ? STDIN_BASE : EMBEDDED_BASE;
    refill();

    log_width = decode_number(CTX_HEADER);
    width = 1 << log_width;
    height = width - decode_number(CTX_HEADER);
    luma_quant = decode_number(CTX_HEADER);
    chroma_quant = decode_number(CTX_HEADER);

    decode_block(0, 0, log_width);

    /* write a binary PPM, converting YCgCo to RGB */
    printf("P6 %d %d 255 ", width, height);
    for (a = 0; a < height * width; a++) {
        luma = planes[0][a];
        cg = planes[1][a];
        co = planes[2][a];
        tmp = luma - cg;
        put_clamped(tmp + co);      /* red */
        put_clamped(luma + cg);     /* green */
        put_clamped(tmp - co);      /* blue */
    }
    return 0;
}
