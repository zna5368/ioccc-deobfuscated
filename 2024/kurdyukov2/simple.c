#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jpeglib.h"
#ifdef _WIN32
#include <fcntl.h>
#endif

/*
 * JPEG Quant Smooth: reduce JPEG compression artifacts by choosing, for every
 * DCT coefficient, a value inside its quantization interval that makes the
 * decoded picture as smooth as possible.  The result is written with all
 * quantization tables set to 1 (quality 100).
 */

#define SAMPLE_RANGE (MAXJSAMPLE + 1)   /* 256 for 8-bit samples */
#define CENTER_SAMPLE (SAMPLE_RANGE / 2)
#define BASIS_SCALE (SAMPLE_RANGE * 2)  /* 512: coefficient used to sample the basis */
#define SMOOTH_PASSES 3

/* fixed point constants of the libjpeg "islow" integer IDCT (scaled by 2^13) */
#define CONST_BITS 13
#define PASS1_BITS 2
#define FIX_0_298631336 2446
#define FIX_0_390180644 3196
#define FIX_0_541196100 4433
#define FIX_0_765366865 6270
#define FIX_0_899976223 7373
#define FIX_1_175875602 9633
#define FIX_1_501321110 12299
#define FIX_1_847759065 15137
#define FIX_1_961570560 16069
#define FIX_2_053119869 16819
#define FIX_2_562915447 20995
#define FIX_3_072711026 25172

/*
 * One 8-point IDCT.  in[k * step] are the eight inputs, out[0..7] receive the
 * eight un-descaled outputs.
 */
static void idct_1d(const int *in, int step, int *out)
{
	int z1, z2, z3, z4, z5;
	int tmp0, tmp1, tmp2, tmp3, tmp10, tmp11, tmp12, tmp13;

	/* even part */
	z2 = in[2 * step];
	z3 = in[6 * step];
	z1 = (z2 + z3) * FIX_0_541196100;
	tmp2 = z1 - z3 * FIX_1_847759065;
	tmp3 = z1 + z2 * FIX_0_765366865;

	tmp0 = (in[0 * step] << CONST_BITS) + (in[4 * step] << CONST_BITS);
	tmp1 = (in[0 * step] << CONST_BITS) - (in[4 * step] << CONST_BITS);

	tmp10 = tmp0 + tmp3;
	tmp13 = tmp0 - tmp3;
	tmp11 = tmp1 + tmp2;
	tmp12 = tmp1 - tmp2;

	/* odd part */
	tmp0 = in[7 * step];
	tmp1 = in[5 * step];
	tmp2 = in[3 * step];
	tmp3 = in[1 * step];

	z1 = tmp0 + tmp3;
	z2 = tmp1 + tmp2;
	z3 = tmp0 + tmp2;
	z4 = tmp1 + tmp3;
	z5 = (z3 + z4) * FIX_1_175875602;

	tmp0 *= FIX_0_298631336;
	tmp1 *= FIX_2_053119869;
	tmp2 *= FIX_3_072711026;
	tmp3 *= FIX_1_501321110;
	z1 *= -FIX_0_899976223;
	z2 *= -FIX_2_562915447;
	z3 = z3 * -FIX_1_961570560 + z5;
	z4 = z4 * -FIX_0_390180644 + z5;

	tmp0 += z1 + z3;
	tmp1 += z2 + z4;
	tmp2 += z2 + z3;
	tmp3 += z1 + z4;

	out[0] = tmp10 + tmp3;
	out[7] = tmp10 - tmp3;
	out[1] = tmp11 + tmp2;
	out[6] = tmp11 - tmp2;
	out[2] = tmp12 + tmp1;
	out[5] = tmp12 - tmp1;
	out[3] = tmp13 + tmp0;
	out[4] = tmp13 - tmp0;
}

/*
 * 8x8 inverse DCT (same arithmetic as libjpeg's jpeg_idct_islow, without
 * dequantization).  The 64 samples are written to out with the given row
 * stride, shifted by CENTER_SAMPLE and clamped to 0..MAXJSAMPLE.
 */
static void idct_8x8(const int *coef, JSAMPLE *out, int stride)
{
	int workspace[64], column[8], result[8];
	int row, col, k, v;

	/* pass 1: columns */
	for (col = 0; col < 8; col++) {
		for (k = 0; k < 8; k++)
			column[k] = coef[k * 8 + col];
		idct_1d(column, 1, result);
		for (k = 0; k < 8; k++)
			workspace[k * 8 + col] = (result[k] + (1 << (CONST_BITS - PASS1_BITS - 1)))
				>> (CONST_BITS - PASS1_BITS);
	}

	/* pass 2: rows */
	for (row = 0; row < 8; row++) {
		idct_1d(workspace + row * 8, 1, result);
		for (k = 0; k < 8; k++) {
			v = (result[k] + (1 << (CONST_BITS + PASS1_BITS + 2)) +
			     (CENTER_SAMPLE << (CONST_BITS + PASS1_BITS + 3)))
				>> (CONST_BITS + PASS1_BITS + 3);
			if (v < 0)
				v = 0;
			else if (v > MAXJSAMPLE)
				v = MAXJSAMPLE;
			out[row * stride + k] = v;
		}
	}
}

static void idct_block(const JCOEF *block, JSAMPLE *out, int stride)
{
	int coef[64], k;

	for (k = 0; k < 64; k++)
		coef[k] = block[k];
	idct_8x8(coef, out, stride);
}

/*
 * Add one pixel pair to the weighted least squares sums.  diff is the current
 * difference between the two pixels, deriv is how much that difference changes
 * per unit of the coefficient.  Pairs whose difference is much larger than the
 * quantization step (probably a real edge) get a small or zero weight.
 */
static void add_pair(float diff, float deriv, int step, float *num, float *den)
{
	float weight = step * 2 + (diff < 0 ? diff : -diff);  /* 2*step - |diff| */

	if (weight < 0)
		weight = 0;
	weight *= weight;
	deriv *= weight;
	*num -= diff * weight * deriv;
	*den += deriv * deriv;
}

int main()
{
	struct jpeg_compress_struct dst;
	struct jpeg_error_mgr jerr;
	JQUANT_TBL *qtable;
	struct jpeg_decompress_struct src;
	jvirt_barray_ptr *coef_arrays;
	float basis[64][64];       /* basis[i][pixel]: pixel change per unit of coefficient i */
	JSAMPLE block_pixels[64];  /* the current block decoded */
	JSAMPLE *image;            /* decoded component with a 1 pixel border */
	int coef[64];
	int i, k, comp, pass, by, bx;
	int width, height, stride, offset;
	int row, col, diag;

#ifdef _WIN32
	_setmode(0, _O_BINARY);
	_setmode(1, _O_BINARY);
#endif

	dst.err = src.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&dst);
	jpeg_create_decompress(&src);
	jpeg_stdio_dest(&dst, stdout);
	jpeg_stdio_src(&src, stdin);
	jpeg_read_header(&src, TRUE);
	coef_arrays = jpeg_read_coefficients(&src);

	/* sample the 64 DCT basis functions */
	for (i = 0; i < 64; i++) {
		for (k = 0; k < 64; k++)
			coef[k] = k == i ? BASIS_SCALE : 0;
		idct_8x8(coef, block_pixels, 8);
		for (k = 0; k < 64; k++)
			basis[i][k] = (block_pixels[k] - CENTER_SAMPLE) / (float)BASIS_SCALE;
	}

	for (comp = 0; comp < src.num_components; comp++) {
		jpeg_component_info *info = &src.comp_info[comp];

		qtable = info->quant_table;
		if (!qtable)
			continue;
		width = info->width_in_blocks * 8;
		info->quant_table = NULL;
		height = info->height_in_blocks * 8;
		stride = width + 2;
		image = malloc((height + 2) * stride * sizeof(*image));
		if (!image)
			continue;

		for (pass = 0; pass < SMOOTH_PASSES; pass++) {
			/* decode the whole component into the image */
			for (by = 0; by < height / 8; by++) {
				JBLOCKROW blocks = *(*src.mem->access_virt_barray)
					((j_common_ptr)&src, coef_arrays[comp], by, 1, TRUE);
				for (bx = 0; bx < width / 8; bx++) {
					JCOEF *c = blocks[bx];
					offset = (by * 8 + 1) * stride + bx * 8 + 1;
					if (pass == 0)  /* dequantize once */
						for (i = 0; i < 64; i++)
							c[i] *= qtable->quantval[i];
					idct_block(c, image + offset, stride);
				}
			}

			/* replicate the edge pixels into the border */
			for (row = 1; row <= height; row++) {
				image[row * stride] = image[row * stride + 1];
				image[row * stride + width + 1] = image[row * stride + width];
			}
			for (col = 0; col < stride; col++) {
				image[col] = image[stride + col];
				image[(height + 1) * stride + col] = image[height * stride + col];
			}

			/* smooth every block */
			for (by = 0; by < height / 8; by++) {
				JBLOCKROW blocks = *(*src.mem->access_virt_barray)
					((j_common_ptr)&src, coef_arrays[comp], by, 1, TRUE);
				for (bx = 0; bx < width / 8; bx++) {
					JCOEF *c = blocks[bx];
					UINT16 *quant = qtable->quantval;
					JSAMPLE *pix = image + (by * 8 + 1) * stride + bx * 8 + 1;
					float dot_new_orig = 0, dot_orig_orig = 0, scale = 0;
					int rescale;

					/*
					 * rescale == 0: move each coefficient toward a smoother picture
					 * rescale == 1: boost the coefficients if they lost energy
					 */
					for (rescale = 0; rescale < 2; rescale++) {
						int changed = 1;

						/* visit AC coefficients by anti-diagonal, high frequencies first */
						for (diag = 14; diag > 0; diag--)
						for (row = diag < 7 ? diag : 7; row >= 0 && diag - row < 8; row--) {
							int step, old, rounded, orig, lo, hi, newval;
							float u;

							col = diag - row;
							i = row * 8 + col;
							step = quant[i];
							if (rescale) {
								old = c[i];
								u = old * scale;
							} else {
								float num = 0, den = 0;
								int a, b;

								/* refresh the decoded block at the start of a diagonal */
								if (changed && row == (diag < 7 ? diag : 7)) {
									idct_block(c, block_pixels, 8);
									changed = 0;
								}
								/* horizontal pairs inside the block */
								if (col)
									for (b = 0; b < 8; b++)
										for (a = 0; a < 7; a++) {
											k = b * 8 + a;
											add_pair(block_pixels[k] - block_pixels[k + 1],
												basis[i][k] - basis[i][k + 1], step, &num, &den);
										}
								/* vertical pairs inside the block */
								if (row)
									for (b = 0; b < 7; b++)
										for (a = 0; a < 8; a++) {
											k = b * 8 + a;
											add_pair(block_pixels[k] - block_pixels[k + 8],
												basis[i][k] - basis[i][k + 8], step, &num, &den);
										}
								/* pairs across the block border: top, bottom, left, right */
								for (a = 0; a < 8; a++)
									add_pair(block_pixels[a] - pix[-stride + a],
										basis[i][a] * 2, step, &num, &den);
								for (a = 0; a < 8; a++)
									add_pair(block_pixels[56 + a] - pix[8 * stride + a],
										basis[i][56 + a] * 2, step, &num, &den);
								for (a = 0; a < 8; a++)
									add_pair(block_pixels[a * 8] - pix[a * stride - 1],
										basis[i][a * 8] * 2, step, &num, &den);
								for (a = 0; a < 8; a++)
									add_pair(block_pixels[a * 8 + 7] - pix[a * stride + 8],
										basis[i][a * 8 + 7] * 2, step, &num, &den);
								u = num / den;
								u += old = c[i];
							}

							/* round half away from zero */
							rounded = u + (u > 0) - 0.5f;

							/* the coefficient that was stored in the file */
							orig = (old + (old < 0 ? -step : step) / 2) / step * step;

							/* range of values that quantize to orig */
							lo = ((orig < 1) - step) / 2 + orig;
							hi = step - (orig ? 1 - lo : step + lo);

							newval = rounded > hi ? hi : rounded < lo ? lo : rounded;
							c[i] = newval;
							changed |= old ^ c[i];

							dot_new_orig += newval * orig;
							dot_orig_orig += orig * orig;
						}

						if (dot_orig_orig > dot_new_orig)
							scale = dot_orig_orig / dot_new_orig;
						else
							break;
					}
				}
			}
		}
		free(image);
	}

	/* the coefficients are now dequantized, so store them with quality 100 */
	for (k = 0; k < NUM_QUANT_TBLS; k++)
		if ((qtable = src.quant_tbl_ptrs[k]))
			for (i = 0; i < 64; i++)
				qtable->quantval[i] = 1;

	jpeg_copy_critical_parameters(&src, &dst);
	jpeg_write_coefficients(&dst, coef_arrays);
	jpeg_finish_compress(&dst);
	jpeg_finish_decompress(&src);
	jpeg_destroy_compress(&dst);
	jpeg_destroy_decompress(&src);
}
