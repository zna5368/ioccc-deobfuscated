#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "jpeglib.h"

/* One block of the mosaic, painted in a single colour. */
struct rect {
    uint16_t x, y, w, h;   /* position and size in pixels               */
    uint8_t color[4];      /* average colour of the pixels it covers     */
    int64_t error;         /* sum of squared differences from that colour */
};

static intptr_t width, height, channels;
static intptr_t row_stride;   /* bytes per image row: width * channels             */
static intptr_t sum_stride;   /* entries per summed-area row: (width + 1) * channels */
static uint8_t *pixels;       /* image, row by row, channels interleaved            */
static int64_t *sums;         /* summed-area table of size (height + 1) * sum_stride */

/*
 * sums[r * sum_stride + c * channels + j] is the total of channel j over all
 * pixels above row r and left of column c.  The first row and column are 0.
 */
static void build_summed_area_table(void)
{
    intptr_t j, row, col;

    for (j = 0; j < channels; j++) {
        for (col = 0; col <= width; col++)
            sums[col * channels + j] = 0;

        for (row = 0; row < height; row++) {
            int64_t *above = sums + row * sum_stride + j;
            int64_t *here = above + sum_stride;
            uint8_t *src = pixels + row * row_stride + j;
            int64_t row_total = 0;

            here[0] = 0;
            for (col = 0; col < width; col++) {
                row_total += src[col * channels];
                here[(col + 1) * channels] = above[(col + 1) * channels] + row_total;
            }
        }
    }
}

/* Work out the average colour of a rectangle and how badly it fits. */
static void update_rect(struct rect *r)
{
    int x0 = r->x, y = r->y, w = r->w, h = r->h, x;
    int64_t *corner = sums + x0 * channels + y * sum_stride;
    intptr_t area = w * h;
    int64_t error = 0;
    intptr_t j, diff;

    for (j = 0; area && j < channels; j++, corner++) {
        int64_t total = corner[w * channels + h * sum_stride] - corner[w * channels]
                      - corner[h * sum_stride] + corner[0];
        r->color[j] = (total + area / 2) / area;   /* rounded mean */
    }

    for (; h--; y++)
        for (x = x0; x < w + x0; x++)
            for (j = 0; j < channels; j++) {
                diff = pixels[y * row_stride + x * channels + j] - r->color[j];
                error += diff * diff;
            }
    r->error = error;
}

/*
 * Try every place to cut a rectangle in two along one direction and return
 * the best score; *best_cut receives where to cut.  "step" moves one pixel
 * along the cut direction, "cross_step" moves one pixel across it, "length"
 * is the size along it and "span" the size across it.
 *
 * The score of a cut is how far the two halves' colour totals are from what
 * the parent's average colour predicts: the bigger the difference, the more
 * the two halves really are different colours.
 */
static int64_t best_split(const struct rect *r, intptr_t step, intptr_t cross_step,
                          intptr_t length, intptr_t span, intptr_t *best_cut)
{
    int64_t best = 0, score;
    intptr_t cut, j;

    *best_cut = 0;
    for (cut = 1; cut < length; cut++) {
        score = 0;
        for (j = 0; j < channels; j++) {
            int64_t *p = sums + r->x * channels + r->y * sum_stride + j;
            int64_t expected = r->color[j] * span;   /* parent mean times one strip */
            int64_t to_cut = p[cut * step + span * cross_step] - p[cut * step];
            int64_t to_end = p[length * step + span * cross_step] - p[length * step];
            int64_t to_start = p[span * cross_step] - p[0];

            int64_t second_dev = to_end - to_cut - expected * (length - cut);
            int64_t first_dev = to_cut - to_start - expected * cut;

            score += second_dev < 0 ? -second_dev : second_dev;
            score += first_dev < 0 ? -first_dev : first_dev;
        }
        if (score > best) {
            best = score;
            *best_cut = cut;
        }
    }
    return best;
}

/* Fill a rectangle of the image with its average colour. */
static void paint_rect(const struct rect *r)
{
    int x0 = r->x, y = r->y, w = r->w, h = r->h, x;
    intptr_t j;

    for (; h--; y++)
        for (x = x0; x < w + x0; x++)
            for (j = 0; j < channels; j++)
                pixels[y * row_stride + x * channels + j] = r->color[j];
}

int main(int argc, char **argv)
{
    intptr_t wanted, count, painted, k;
    int64_t total_bytes, worst_error;
    struct rect *rects;
    JSAMPROW row;
    FILE *file;

    if (argc != 4)
        return 1;
    wanted = atoi(argv[1]);

    /* ---- read the input JPEG ---- */
    {
        struct jpeg_error_mgr jerr;
        struct jpeg_decompress_struct cinfo;

        cinfo.err = jpeg_std_error(&jerr);
        if (!(file = fopen(argv[2], "rb")))
            return 1;
        jpeg_create_decompress(&cinfo);
        jpeg_stdio_src(&cinfo, file);
        jpeg_read_header(&cinfo, TRUE);
        cinfo.do_fancy_upsampling = FALSE;
        cinfo.out_color_space = JCS_RGB;
        jpeg_start_decompress(&cinfo);

        width = cinfo.output_width;
        height = cinfo.output_height;
        channels = cinfo.output_components;

        if (wanted < 1)
            wanted = 1;
        else if (wanted > width * height)
            wanted = width * height;

        row_stride = width * channels;
        sum_stride = row_stride + channels;

        /* one block: summed-area table, then rectangles, then pixels */
        intptr_t sum_entries = sum_stride * (height + 1);
        total_bytes = sizeof(struct rect) * wanted + sum_entries * 9;
        if ((int64_t)(size_t)total_bytes != total_bytes || !(sums = malloc(total_bytes)))
            return 1;
        rects = (struct rect *)(sums + sum_entries);
        pixels = (uint8_t *)(rects + wanted);

        while ((intptr_t)cinfo.output_scanline < height) {
            row = pixels + cinfo.output_scanline * row_stride;
            jpeg_read_scanlines(&cinfo, &row, 1);
        }
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        fclose(file);
    }

    build_summed_area_table();

    /* start with one rectangle covering the whole image */
    rects[0] = (struct rect){0, 0, width, height, {0}, 0};
    update_rect(&rects[0]);
    count = 1;
    worst_error = 1;

    /* keep splitting the worst fitting rectangle in two */
    while (worst_error && count < wanted) {
        struct rect *worst = rects, piece;
        intptr_t cut_x, cut_y;
        int64_t score_x, score_y;

        worst_error = 0;
        for (k = 0; k < count; k++)
            if (rects[k].error > worst_error) {
                worst = &rects[k];
                worst_error = worst->error;
            }

        piece = *worst;
        score_x = best_split(worst, channels, sum_stride, worst->w, worst->h, &cut_x);
        score_y = best_split(worst, sum_stride, channels, worst->h, worst->w, &cut_y);

        if (score_x < score_y) {        /* horizontal cut: worst keeps the top */
            piece.y += worst->h = cut_y;
            piece.h -= cut_y;
        } else {                        /* vertical cut: worst keeps the left */
            piece.x += worst->w = cut_x;
            piece.w -= cut_x;
        }
        rects[count] = piece;
        update_rect(&rects[count]);
        update_rect(worst);
        count++;
    }

    /* the original skips the newest rectangle when it stopped on a perfect fit */
    painted = worst_error ? count : count - 1;
    for (k = 0; k < painted; k++)
        paint_rect(&rects[k]);

    /* ---- write the output JPEG ---- */
    {
        struct jpeg_error_mgr jerr;
        struct jpeg_compress_struct cinfo;

        cinfo.err = jpeg_std_error(&jerr);
        if (!(file = fopen(argv[3], "wb")))
            return 1;
        jpeg_create_compress(&cinfo);
        jpeg_stdio_dest(&cinfo, file);
        cinfo.input_components = channels;
        cinfo.in_color_space = JCS_RGB;
        jpeg_set_defaults(&cinfo);
        cinfo.comp_info[0].h_samp_factor = 1;
        cinfo.image_width = width;
        cinfo.comp_info[0].v_samp_factor = 1;
        cinfo.image_height = height;
        cinfo.optimize_coding = TRUE;
        jpeg_set_quality(&cinfo, 100, TRUE);
        jpeg_start_compress(&cinfo, TRUE);
        while ((intptr_t)cinfo.next_scanline < height) {
            row = pixels + cinfo.next_scanline * row_stride;
            jpeg_write_scanlines(&cinfo, &row, 1);
        }
        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        fclose(file);
    }

    free(sums);
    return 0;
}
