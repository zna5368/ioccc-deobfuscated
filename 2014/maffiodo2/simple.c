/*
 * The original has no #include lines: getchar() and putchar() are declared
 * implicitly, and the Makefile supplies strlen() with -include string.h.
 */

/* Number of pixels printed on each line of the output */
#define OUTPUT_WIDTH 80

/* A colour channel has 256 possible values, so dividing by 256 is a shift by 8 */
#define CHANNEL_BITS 8

int main(int argc, char **argv)
{
    /* Characters to draw with, ordered from the darkest pixel to the brightest */
    char *ramp = argv[1];
    int pixels_printed = 0;

    for (;;) {
        int channel_sum;
        int brightness;

        /* Only the red byte is checked for the end of the input */
        channel_sum = getchar();
        if (channel_sum < 0) {
            break;
        }

        /*
         * If the input stops in the middle of a pixel then each missing byte
         * adds the -1 returned by getchar(). image.rgb ends this way.
         */
        channel_sum += getchar();
        channel_sum += getchar();

        /* The average of red, green and blue is the brightness from 0 to 255 */
        brightness = channel_sum / 3;

        /*
         * Scale the brightness to a position in the ramp. The ramp is measured
         * here rather than before the loop because a missing argument only
         * crashes when there is a pixel to draw.
         */
        putchar(ramp[brightness * strlen(ramp) >> CHANNEL_BITS]);

        pixels_printed++;
        if (pixels_printed % OUTPUT_WIDTH == 0) {
            putchar('\n');
        }
    }
}
