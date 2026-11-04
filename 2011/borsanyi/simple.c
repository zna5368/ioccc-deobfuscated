#include <math.h>
#include <stdio.h>

/*
 * fork() and wait() are used without their headers (<unistd.h> and
 * <sys/wait.h>), exactly like the original entry.
 */

#define NUM_BINS 22        /* approximate number of histogram bins */
#define HASHES_FOR_ALL 400 /* number of '#' a bin holding every point gets */

double bin_width; /* width of every bin */
double bin_start; /* lower (exclusive) edge of the bin this process counts */
int num_points;   /* total number of data points read */

/*
 * Decide the bin layout from the data range, then fork one process per bin.
 * Every process leaves this function with bin_start set to its own bin.
 */
void make_bins(double min, double max)
{
    double unit;
    int bins_left;

    /* round the bin width down to three significant digits */
    bin_width = (max - min) / NUM_BINS;
    unit = pow(10, (int)(log10(bin_width)) - 2);
    bin_width = unit * (int)(bin_width / unit);

    /* the first bin is centered on the minimum value */
    bin_start = min - bin_width / 2;
    bins_left = (int)((bin_width + max - min) / bin_width);

    /*
     * The parent keeps the current bin; the child moves on to the next one
     * and forks again, until the last bin is reached.
     */
    while (bins_left > 1 && !fork()) {
        bin_start += bin_width;
        bins_left--;
    }
}

/*
 * Read one data point per recursive call, so the points are stored in the
 * stack frames instead of an array.  Once the input is exhausted, the bins
 * are made (forking the process, stack and all), and on the way back up each
 * frame checks whether its point falls into this process's bin.
 *
 * min > max means that no point has been read yet.
 */
int read_points(double min, double max)
{
    double value;
    int count;

    if (scanf("%lf", &value) != 1) {
        make_bins(min, max);
        return 0;
    }
    num_points++;

    count = read_points(min > max | value < min ? value : min,
                        min > max | value > max ? value : max);

    if (value > bin_start && value <= bin_start + bin_width)
        count++;

    return count;
}

int main(void)
{
    int count;
    int hashes;

    count = read_points(1, 0);

    /* let the process of the next bin print first */
    wait(NULL);

    printf("%+12g %4d", bin_start + bin_width / 2, count);
    for (hashes = count * HASHES_FOR_ALL / num_points; hashes > 0; hashes--)
        putchar('#');
    putchar('\n');

    return 0;
}
