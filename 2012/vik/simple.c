#include <stdio.h>
#include <string.h>
#include "zlib.h"

#define BUFFER_SIZE (1 << 24)
#define LARGE_BUFFER_SIZE (2 << 24)

#define PNG_SIGNATURE_START 0x89504e47
#define PNG_SIGNATURE_SIZE 8
#define PNG_CHUNK_OVERHEAD 12 /* length + type + CRC */
#define PNG_CHUNK_DATA 8      /* offset of the data inside a chunk */
#define PNG_CHUNK_TYPE_SIZE 4
#define PNG_CHUNK_TYPE_LETTER 5 /* offset of the second letter of the type */

/* Offsets into a PNG file, which always starts with the IHDR chunk */
#define IHDR_TYPE 12
#define IHDR_WIDTH 16
#define IHDR_HEIGHT 20
#define IHDR_COLOR_TYPE 25
#define IHDR_CRC 29
#define IHDR_CRC_LENGTH 17 /* type + 13 data bytes */

#define COLOR_FLAG_RGB 2
#define COLOR_FLAG_ALPHA 4
#define COLOR_TYPE_RGB 2

#define FILTER_NONE 0
#define FILTER_SUB 1
#define FILTER_UP 2
#define FILTER_AVERAGE 3

#define BYTES_PER_PIXEL 3
#define BITS_PER_PIXEL_BYTE 2 /* hidden bits stored in every colour byte */
#define BITS_PER_BYTE 8
#define HIDDEN_BITS_MASK 3
#define HIDDEN_BITS_SHIFT 6
#define RESTORED_LOW_BITS 56 /* fills the bits that were never stored */

#define BRAINFUCK_MAX_DEPTH 990

static const char text_marker[] = "IDAT"; /* also the PNG data chunk's type */

static int row_bytes;   /* width of the last PNG in RGB bytes */
static int image_bytes; /* total RGB bytes of the last PNG */
static int color_type;  /* color type the last PNG was stored with */
static unsigned char *chunks_end; /* just past the last non IDAT chunk */

static unsigned char base_file[BUFFER_SIZE];    /* the output PNG, in place */
static unsigned char base_pixels[BUFFER_SIZE];  /* also rows to compress */
/* A text marker followed by the embedded file or the inflated image data */
static unsigned char payload[BUFFER_SIZE];
/* The result image, then compressed data. The top half is the brainfuck tape */
static unsigned char hidden_pixels[LARGE_BUFFER_SIZE];
/* The data taken from the image. The top half collects the IDAT chunks */
static unsigned char extracted[LARGE_BUFFER_SIZE];

static int read_big_endian(const unsigned char *bytes)
{
    return (int)((unsigned)bytes[0] << 24 | (unsigned)bytes[1] << 16 |
                 (unsigned)bytes[2] << 8 | (unsigned)bytes[3]);
}

static void write_big_endian(unsigned char *bytes, uLong value)
{
    bytes[0] = value >> 24;
    bytes[1] = value >> 16;
    bytes[2] = value >> 8;
    bytes[3] = value;
}

static int absolute(int value)
{
    return value > 0 ? value : -value;
}

static int paeth_predictor(int left, int up, int upper_left)
{
    int left_distance = absolute(up - upper_left);
    int up_distance = absolute(left - upper_left);
    int upper_left_distance = absolute(left + up - 2 * upper_left);

    if (left_distance > up_distance || left_distance > upper_left_distance) {
        return up_distance > upper_left_distance ? upper_left : up;
    }
    return left;
}

/* Undo the PNG scanline filters, always producing 3 bytes for every pixel */
static void unfilter_scanlines(const unsigned char *filtered,
                               unsigned char *pixels)
{
    int filter = FILTER_NONE;
    int index;

    for (index = 0; index < image_bytes; index++) {
        int column = index % row_bytes;
        int first_row = index / row_bytes == 0;

        if (column == 0) {
            filter = *filtered++;
        }

        if (!(color_type & COLOR_FLAG_RGB) && column % BYTES_PER_PIXEL != 0) {
            /* Greyscale stores one byte, which is repeated for green, blue */
            *pixels = pixels[-1];
        } else {
            int first_pixel = column < BYTES_PER_PIXEL;
            int left = first_pixel ? 0 : pixels[-BYTES_PER_PIXEL];
            int up = first_row ? 0 : pixels[-row_bytes];
            int upper_left = first_row || first_pixel
                                 ? 0
                                 : pixels[-BYTES_PER_PIXEL - row_bytes];
            int prediction;

            switch (filter) {
            case FILTER_NONE:
                prediction = 0;
                break;
            case FILTER_SUB:
                prediction = left;
                break;
            case FILTER_UP:
                prediction = up;
                break;
            case FILTER_AVERAGE:
                prediction = (left + up) / 2;
                break;
            default:
                prediction = paeth_predictor(left, up, upper_left);
                break;
            }
            *pixels = *filtered++ + prediction;
        }
        pixels++;

        /* The alpha byte follows each pixel and is thrown away */
        if ((index + 1) % BYTES_PER_PIXEL == 0 &&
            (color_type & COLOR_FLAG_ALPHA)) {
            filtered++;
        }
    }
}

/* Load a file, and when there is no such file use the name itself as text */
static int load_file_or_text(unsigned char *buffer, const char *name)
{
    FILE *file = fopen(name, "rb");
    int size;

    if (file == NULL) {
        strcpy((char *)buffer, name);
        return 0;
    }
    size = fread(buffer, 1, BUFFER_SIZE, file);
    fclose(file);
    buffer[size] = 0;
    return size;
}

/*
 * Load a file into buffer. For a PNG, the header is rewritten to say RGB, the
 * IDAT chunks are cut out and the RGB bytes are decoded into pixels. Returns
 * the number of RGB bytes, which is negated when the file is not a PNG.
 */
static int load_image(unsigned char *buffer, const char *name,
                      unsigned char *pixels)
{
    unsigned char *compressed_start = extracted + BUFFER_SIZE;
    unsigned char *compressed_end = compressed_start;
    unsigned char *chunk = buffer + PNG_SIGNATURE_SIZE;
    int remaining = load_file_or_text(buffer, name);
    uLong inflated_size = BUFFER_SIZE;

    if ((unsigned)read_big_endian(buffer) != PNG_SIGNATURE_START) {
        return -image_bytes;
    }

    row_bytes = read_big_endian(buffer + IHDR_WIDTH) * BYTES_PER_PIXEL;
    image_bytes = read_big_endian(buffer + IHDR_HEIGHT) * row_bytes;
    color_type = buffer[IHDR_COLOR_TYPE];
    buffer[IHDR_COLOR_TYPE] = COLOR_TYPE_RGB;
    write_big_endian(buffer + IHDR_CRC,
                     crc32(0, buffer + IHDR_TYPE, IHDR_CRC_LENGTH));

    /* Walk the chunks, treating any type with a second letter D as IDAT */
    do {
        int size = read_big_endian(chunk) + PNG_CHUNK_OVERHEAD;

        remaining -= size;
        if (chunk[PNG_CHUNK_TYPE_LETTER] != 'D') {
            chunk += size;
            chunks_end = chunk;
        } else {
            int data_size = size - PNG_CHUNK_OVERHEAD;

            memcpy(compressed_end, chunk + PNG_CHUNK_DATA, data_size);
            compressed_end += data_size;
            memmove(chunk, chunk + size, remaining);
        }
    } while (remaining > PNG_SIGNATURE_SIZE);

    uncompress(payload + PNG_CHUNK_TYPE_SIZE, &inflated_size, compressed_start,
               compressed_end - compressed_start);
    unfilter_scanlines(payload + PNG_CHUNK_TYPE_SIZE, pixels);
    return image_bytes;
}

/* Split data into two bit pieces, keeping each in the top of its own byte */
static void spread_bits(const unsigned char *data, unsigned char *pieces,
                        int count)
{
    int bit;

    for (bit = 0; bit < count * BITS_PER_PIXEL_BYTE;
         bit += BITS_PER_PIXEL_BYTE) {
        *pieces++ = data[bit / BITS_PER_BYTE] << bit % BITS_PER_BYTE;
    }
}

/* Replace the low bits of the base image with the top bits of each piece */
static void hide_bits(unsigned char *pieces, const unsigned char *pixels,
                      int count)
{
    int index;

    for (index = 0; index < count; index++) {
        pieces[index] = pieces[index] >> HIDDEN_BITS_SHIFT |
                        (pixels[index] & ~HIDDEN_BITS_MASK);
    }
}

/*
 * Take the low bits of every byte in the image. They are both turned into a
 * viewable image in restored and packed back together into data.
 */
static void extract_bits(const unsigned char *pixels, unsigned char *restored,
                         unsigned char *data, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        int bit = index * BITS_PER_PIXEL_BYTE;
        int piece = (pixels[index] & HIDDEN_BITS_MASK) << HIDDEN_BITS_SHIFT;

        restored[index] = piece | RESTORED_LOW_BITS;
        data[bit / BITS_PER_BYTE] += piece >> bit % BITS_PER_BYTE;
    }
}

/* Put a "no filter" byte before every row, returning the number of rows */
static int add_filter_bytes(unsigned char *rows, const unsigned char *pixels,
                            const unsigned char *pixels_end)
{
    int row_count = 0;

    do {
        *rows++ = FILTER_NONE;
        memcpy(rows, pixels, row_bytes);
        rows += row_bytes;
        pixels += row_bytes;
        row_count++;
    } while (pixels != pixels_end);

    return row_count;
}

/*
 * Insert a new IDAT chunk holding the pixels in front of the IEND chunk at the
 * offset given. Returns the size of the completed PNG file.
 */
static int insert_image_data(int iend_offset, const unsigned char *pixels,
                             int pixel_count)
{
    unsigned char *chunk = base_file + iend_offset;
    int rows_size = pixel_count + add_filter_bytes(base_pixels, pixels,
                                                   pixels + pixel_count);
    uLong compressed_size = BUFFER_SIZE;
    int data_size;

    compress(hidden_pixels, &compressed_size, base_pixels, rows_size);
    data_size = compressed_size;

    memmove(chunk + data_size + PNG_CHUNK_OVERHEAD, chunk, PNG_CHUNK_OVERHEAD);
    memcpy(chunk + PNG_CHUNK_DATA, hidden_pixels, data_size);
    write_big_endian(chunk, data_size);
    memcpy(chunk + PNG_CHUNK_TYPE_SIZE, text_marker, PNG_CHUNK_TYPE_SIZE);
    write_big_endian(chunk + PNG_CHUNK_DATA + data_size,
                     crc32(0, chunk + PNG_CHUNK_TYPE_SIZE,
                           data_size + PNG_CHUNK_TYPE_SIZE));

    return iend_offset + data_size + 2 * PNG_CHUNK_OVERHEAD;
}

/*
 * A tiny interpreter. "[" never skips ahead, and a "]" that finishes its loop
 * leaves the loop on the stack.
 */
static void run_brainfuck(const unsigned char *program,
                          const unsigned char *program_end)
{
    const unsigned char *loop_starts[BRAINFUCK_MAX_DEPTH];
    unsigned char *cell = hidden_pixels + BUFFER_SIZE;
    int depth = 0;

    while (program != program_end) {
        switch (*program) {
        case '>':
            cell++;
            break;
        case '<':
            cell--;
            break;
        case '+':
            ++*cell;
            break;
        case '-':
            --*cell;
            break;
        case '.':
            putchar(*cell);
            break;
        case ',':
            *cell = getchar();
            break;
        case '[':
            loop_starts[depth++] = program;
            break;
        case ']':
            if (depth != 0 && *cell != 0) {
                program = loop_starts[--depth];
                continue;
            }
            break;
        }
        program++;
    }
}

int main(int argc, char **argv)
{
    char mode = argv[1][0];
    char *base_name = argv[2];
    char *source_name = argv[3];
    int iend_offset;
    int pixel_count;

    (void)argc;

    /* The embedded data starts with this when it is not a PNG */
    memcpy(payload, text_marker, PNG_CHUNK_TYPE_SIZE);

    pixel_count = load_image(base_file, base_name, base_pixels);
    iend_offset = chunks_end - base_file - PNG_CHUNK_OVERHEAD;

    if (mode == 'e') {
        pixel_count = load_image(payload + PNG_CHUNK_TYPE_SIZE, source_name,
                                 hidden_pixels);
        if (pixel_count < 0) {
            pixel_count = -pixel_count;
            spread_bits(payload, hidden_pixels, pixel_count);
        }
        hide_bits(hidden_pixels, base_pixels, pixel_count);
    } else {
        extract_bits(base_pixels, hidden_pixels, extracted, pixel_count);
    }

    if (mode == 'b') {
        run_brainfuck(extracted, extracted + image_bytes);
        return 0;
    }

    /* The exit status is the number of bytes written */
    if (memcmp(extracted, payload, PNG_CHUNK_TYPE_SIZE) == 0) {
        char *text = (char *)extracted + PNG_CHUNK_TYPE_SIZE;

        return fwrite(text, 1, strlen(text), stdout);
    }
    return fwrite(base_file, 1,
                  insert_image_data(iend_offset, hidden_pixels, pixel_count),
                  stdout);
}
