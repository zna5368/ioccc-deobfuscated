#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*
 * Render an animated GIF of a (p,q) torus knot.  The knot is drawn as a tube
 * made of small quadrilaterals that are scanline rasterised with a z-buffer,
 * shaded from the surface normal, and dithered down to a 128 colour palette.
 */

#define IMAGE_SIZE 256           /* width and height of the GIF in pixels   */
#define KNOT_STEPS 80            /* mesh rings along the knot curve         */
#define TUBE_STEPS 20            /* mesh vertices around the tube           */
#define DEFAULT_FRAMES 40

#define KNOT_SCALE 25.0          /* size of the torus in pixels             */
#define TUBE_RADIUS 15.0         /* radius of the tube cross section        */
#define DEPTH_OFFSET 65536       /* keeps every depth value negative        */

/* GIF LZW codes for a minimum code size of 7 (codes are 8 bits wide) */
#define LZW_MIN_CODE_SIZE 7
#define LZW_CLEAR 128
#define LZW_END 129
#define LZW_FIRST_FREE 130       /* first table entry the decoder will add  */
#define LZW_LAST_8BIT 255        /* after this the code width would grow    */
#define BACKGROUND_COLOR 127     /* palette entry 127 is white              */

/* layout of one mesh vertex */
enum { V_ROW, V_DEPTH, V_COLUMN, V_RED, V_GREEN, V_BLUE, V_FIELDS };

/* layout of one z-buffer pixel / interpolated span point */
enum { P_DEPTH, P_RED, P_GREEN, P_BLUE, P_FIELDS };
#define P_COLUMN P_DEPTH         /* a span point keeps its column in slot 0 */

typedef double Vector[3];
typedef double Pixel[P_FIELDS];

static int knot_p, knot_q;
static double mesh[KNOT_STEPS][TUBE_STEPS][V_FIELDS];
static int dither[64];           /* 8x8 ordered dithering (Bayer) matrix    */

/* moving frame along the knot curve */
static Vector center, tangent, normal;

/* GIF data sub-block buffer: byte 0 is the length, then up to 255 bytes */
static char block[256];
static int block_length;

/*
 * The angle step for dividing a full turn into `steps` pieces.
 * (25 / ln 36)^4 / 377 happens to be a close approximation of 2 * pi.
 */
static double turn_step(int steps)
{
	return pow(25 / log(36), 4) / steps / 377;
}

/*
 * Recursively fill an 8x8 Bayer threshold matrix with the values 0..63.
 * Each level splits the square into four quadrants and gives them
 * increasingly finer threshold offsets.
 */
static void build_dither(int *cell, int level, int value)
{
	static const int quadrant_offset[4] = { 0, 1, 8, 9 };
	static const int quadrant_rank[4] = { 0, 3, 2, 1 };
	int quadrant;

	*cell = value;
	for (quadrant = 0; level && quadrant < 4; quadrant++)
		build_dither(cell + quadrant_offset[quadrant] * (1 << (level - 1)),
			     level - 1,
			     value + (1 << (6 - 2 * level)) * quadrant_rank[quadrant]);
}

static void put_bytes(const unsigned char *bytes, size_t count)
{
	size_t i;

	for (i = 0; i < count; i++)
		putchar(bytes[i]);
}

/* Append one 8 bit LZW code to the current data sub-block. */
static void emit_code(int code)
{
	block[++block_length] = code;
	if (block_length > 254 || code == LZW_END) {
		block[0] = block_length;
		fwrite(block, block_length + 1, 1, stdout);
		block_length = 0;
	}
}

static void normalize(double *vec)
{
	double length = sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
	int k;

	for (k = 0; k < 3; k++)
		vec[k] = vec[k] / length;
}

/*
 * Compute a point on the torus knot at parameter s together with the unit
 * tangent and a unit normal, which span the tube's cross section.
 */
static void knot_frame(double s)
{
	double neg_sin_p = -sin(knot_p * s), cos_p = cos(knot_p * s);
	double sin_q = sin(knot_q * s), cos_q = cos(knot_q * s);
	double dot;
	int k;

	center[0] = (cos_p + 3) * cos_q;
	center[1] = (cos_p + 3) * sin_q;
	center[2] = -neg_sin_p;
	memcpy(normal, center, sizeof normal);
	normalize(normal);

	/* derivative of the curve with respect to s */
	tangent[0] = knot_p * (neg_sin_p * cos_q) + knot_q * -center[1];
	tangent[1] = knot_p * (neg_sin_p * sin_q) + knot_q * center[0];
	tangent[2] = knot_p * cos_p;
	normalize(tangent);

	/* make the normal perpendicular to the tangent (Gram-Schmidt) */
	dot = tangent[0] * normal[0] + tangent[1] * normal[1] +
	      tangent[2] * normal[2];
	for (k = 0; k < 3; k++)
		normal[k] = normal[k] - dot * tangent[k];
	normalize(normal);
}

/*
 * Compute one vertex on the tube surface at angle t around the current knot
 * frame, rotate it by the animation angle and project it to the screen.
 */
static void tube_vertex(double *vertex, double t, double rotation)
{
	Vector surface_normal, point;
	double cos_r = cos(rotation), sin_r = sin(rotation);
	int k;

	for (k = 0; k < 3; k++) {
		int k1 = (k + 1) % 3, k2 = (k + 2) % 3;
		double binormal = normal[k1] * tangent[k2] - normal[k2] * tangent[k1];

		surface_normal[k] = cos(t) * normal[k] + sin(t) * binormal;
		point[k] = KNOT_SCALE * center[k] + TUBE_RADIUS * surface_normal[k];
	}

	/* rotate around the vertical axis and move to the image center */
	vertex[V_ROW] = point[1] + IMAGE_SIZE / 2;
	vertex[V_DEPTH] = -point[0] * sin_r + point[2] * cos_r - DEPTH_OFFSET;
	vertex[V_COLUMN] = (point[0] * cos_r + point[2] * sin_r) + IMAGE_SIZE / 2;

	/* each colour channel is lit by a light from a different direction */
	vertex[V_RED] = fabs(surface_normal[1] + surface_normal[2]) / sqrt(2);
	vertex[V_GREEN] = fabs(surface_normal[0] + surface_normal[1] +
			       surface_normal[2]) / sqrt(3);
	vertex[V_BLUE] = fabs(surface_normal[0] + surface_normal[2]) / sqrt(2);
}

static void build_mesh(double rotation)
{
	double s, t, s_step = turn_step(KNOT_STEPS), t_step;
	int ring, j;

	for (s = ring = 0; ring < KNOT_STEPS; ring++, s += s_step) {
		knot_frame(s);
		t_step = turn_step(TUBE_STEPS);
		for (t = j = 0; j < TUBE_STEPS; j++, t += t_step)
			tube_vertex(mesh[ring][j], t, rotation);
	}
}

/*
 * Rasterise every quad of the mesh that crosses scanline y into the given
 * row of z-buffered pixels.
 */
static void draw_scanline(Pixel *row, int y)
{
	/* corners of a quad in order, ending back at the first corner */
	static const int corner_ring[5] = { 0, 1, 1, 0, 0 };
	static const int corner_around[5] = { 0, 0, 1, 1, 0 };
	Pixel crossing, previous = { 0 }, gradient;
	double *from = NULL, *to;
	int quad, corner, h, span;

	for (quad = 0; quad < KNOT_STEPS * TUBE_STEPS; quad++) {
		int ring = quad / TUBE_STEPS, around = quad % TUBE_STEPS;
		double depth = mesh[ring][around][V_DEPTH];

		previous[P_COLUMN] = 0;
		for (corner = 0; corner < 5; corner++, from = to) {
			to = mesh[(ring + corner_ring[corner]) % KNOT_STEPS]
				 [(around + corner_around[corner]) % TUBE_STEPS];

			/* does the edge from -> to cross this scanline? */
			if (!corner || !((y < from[V_ROW]) ^ (y < to[V_ROW])))
				continue;

			/* interpolate column and colours at the crossing */
			for (h = 0; h < P_FIELDS; h++) {
				crossing[h] = from[h + V_COLUMN] +
					      (to[h + V_COLUMN] - from[h + V_COLUMN]) *
						      (y - from[V_ROW]) /
						      (to[V_ROW] - from[V_ROW]);
				if (h == 0) {
					crossing[P_COLUMN] = (int)crossing[P_COLUMN];
					span = 1 + fabs(previous[P_COLUMN] -
							crossing[P_COLUMN]);
				} else if (previous[P_COLUMN]) {
					gradient[h] = (previous[h] - crossing[h]) / span;
				}
			}

			/* fill the span between this crossing and the previous one */
			if (previous[P_COLUMN]) {
				Pixel *pixel = row + (int)crossing[P_COLUMN];
				int direction = crossing[P_COLUMN] < previous[P_COLUMN] ? 1 : -1;

				while (span--) {
					if ((*pixel)[P_DEPTH] > depth) {
						memcpy(*pixel, crossing, sizeof crossing);
						(*pixel)[P_DEPTH] = depth;
					}
					for (h = 1; h < P_FIELDS; h++)
						crossing[h] += gradient[h];
					pixel += direction;
				}
			}
			memcpy(previous, crossing, sizeof previous);
		}
	}
}

/*
 * Reduce a pixel's colour to a 7 bit palette index: 2 bits red, 3 bits
 * green and 2 bits blue, using ordered dithering between levels.
 */
static int dithered_color(Pixel pixel, int x, int y)
{
	int color = 0, channel;

	for (channel = P_BLUE; channel >= P_RED; channel--) {
		int levels = channel == P_GREEN ? 7 : 3;
		int bits = channel == P_GREEN ? 3 : 2;
		double level = pixel[channel] * levels;
		int threshold = dither[(y & 7) * 8 + (x & 7)];
		int quantized = level + (63 * (level - (int)level) > threshold);

		if (quantized > levels)
			quantized = levels;
		color = color << bits | (quantized & levels);
	}
	return color;
}

static void write_frame(double rotation)
{
	static const unsigned char image_descriptor[] = {
		',', 0, 0, 0, 0,                 /* left, top          */
		0, 1, 0, 1,                      /* 256 x 256          */
		0,                               /* no local palette   */
		LZW_MIN_CODE_SIZE
	};
	Pixel row[IMAGE_SIZE];
	int x, y, next_code;

	fputc('.', stderr);
	build_mesh(rotation);

	put_bytes(image_descriptor, sizeof image_descriptor);
	next_code = LZW_FIRST_FREE;
	emit_code(LZW_CLEAR);

	for (y = 0; y < IMAGE_SIZE; y++) {
		memset(row, 0, sizeof row);   /* depth 0 means nothing drawn */
		draw_scanline(row, y);

		/*
		 * Every pixel is sent as a literal code.  A clear code is sent
		 * before the decoder's table would need 9 bit codes.
		 */
		for (x = 0; x < IMAGE_SIZE; x++) {
			emit_code(row[x][P_DEPTH] ? dithered_color(row[x], x, y)
						  : BACKGROUND_COLOR);
			if (next_code < LZW_LAST_8BIT) {
				next_code++;
			} else {
				emit_code(LZW_CLEAR);
				next_code = LZW_FIRST_FREE;
			}
		}
	}
	emit_code(LZW_END);
	putchar(0);                           /* block terminator */
}

int main(int argc, char **argv)
{
	static const unsigned char header[] = {
		'G', 'I', 'F', '8', '9', 'a',
		0, 1, 0, 1,     /* 256 x 256 */
		0xE6,           /* global palette of 128 colours, 7 bit colour */
		0,              /* background colour */
		0               /* pixel aspect ratio */
	};
	static const unsigned char loop_extension[] = {
		'!', 0xFF, 11, 'N', 'E', 'T', 'S', 'C', 'A', 'P', 'E', '2', '.', '0',
		3, 1, 0, 0,     /* loop forever */
		0
	};
	int frames, frame, n;
	double rotation, rotation_step;

	if (argc < 3)
		return fprintf(stderr, "Usage: %s P Q [F]\n", *argv), 1;

	knot_p = atoi(argv[1]);
	knot_q = atoi(argv[2]);
	build_dither(dither, 3, 0);

	put_bytes(header, sizeof header);

	/* palette: index bits are bbgggrr, unused low bits set */
	for (n = 0; n < 128; n++) {
		putchar((n & 3) << 6 | 63);
		putchar((n >> 2 & 7) << 5 | 31);
		putchar((n >> 5 & 3) << 6 | 63);
	}

	put_bytes(loop_extension, sizeof loop_extension);

	frames = argc > 3 ? atoi(argv[3]) : DEFAULT_FRAMES;
	rotation_step = turn_step(frames);
	for (rotation = frame = 0; frame < frames; frame++, rotation += rotation_step)
		write_frame(rotation);

	putchar(';');                         /* GIF trailer */

	return 0;
}
