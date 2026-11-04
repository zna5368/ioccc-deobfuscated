#include <stdlib.h>
#include <stdio.h>

/*
 * The picture is a GRID x GRID square drawn with a MARGIN around it.
 * A site at (x, y) in the unit square is drawn at MARGIN + GRID * (x, y).
 */
#define MARGIN 15
#define GRID 512
#define WORKSPACE_SIZE 1000000 /* number of doubles in the work buffer */

/*
 * Everything lives in one zeroed work buffer of doubles:
 *
 *   sites[0 .. 2*(site_count+2))    (x, y) pairs sorted by x, including two
 *                                   sentinel sites far outside the picture
 *   colors[0 .. 4*(site_count+2))   4 random bytes per site (r, g, b, unused),
 *                                   placed right after the sites
 *   ...
 *   input_copy (top of the buffer)  every value read, stored backwards so the
 *                                   labels can be printed in input order
 */
double *sites;
int site_count;

static const char *palette[] = {"ivory", "black"}; /* label colors */
static unsigned char averaged_color[3];

/* qsort comparator: order sites by their x coordinate */
int compare_x(const void *a, const void *b)
{
    double xa = *(const double *)a, xb = *(const double *)b;
    return (xa > xb) - (xa < xb);
}

/*
 * Squared distance from point (px, py) to the site.  The squared horizontal
 * distance alone is also stored in *dx_squared.
 */
double distance_squared(double px, double py, const double *site,
                        double *dx_squared)
{
    double dx = site[0] - px;
    double dy = py - site[1];
    *dx_squared = dx * dx;
    return *dx_squared + dy * dy;
}

/*
 * Color of a site, picked at random the first time it is needed.
 * A color whose first byte (red) is zero is rejected and drawn again.
 */
unsigned char *site_color(const double *site)
{
    unsigned char *colors = (unsigned char *)(sites + 2 * (site_count + 2));
    unsigned char *color = colors + 4 * ((site - sites) / 2);

    while (color[0] == 0) {
        color[3] = rand();
        color[1] = rand();
        color[2] = rand();
        color[0] = rand();
    }
    return color;
}

/*
 * Rough binary search over the x-sorted sites for a site whose x is close to
 * x.  `first` and `last` are pointers into the array of doubles.
 */
double *find_start_site(double *first, double *last, double x)
{
    int half = (last - first) / 4 * 2;

    if (half <= 0)
        return first;
    if (first[half] < x)
        return find_start_site(first + half, last, x);
    return find_start_site(first, first + half, x);
}

/*
 * Nearest site to the picture point (u, w).
 *
 * Start from a site whose x is near the point, then walk outward through the
 * x-sorted sites, one walker going right and one going left.  The walker that
 * is horizontally closer is always moved next, and the walk stops once both
 * walkers are horizontally farther away than the best distance found.
 */
double *nearest_site(double u, double w)
{
    double x = u / (GRID - 1);
    double y = w / (GRID - 1);
    double *best = find_start_site(sites, sites + site_count + 2, x);
    double dx_squared;
    double best_distance = distance_squared(x, y, best, &dx_squared);

    double *walker[2] = {best, best};             /* 0 goes right, 1 goes left */
    double walker_dx[2] = {dx_squared, dx_squared}; /* squared x distance */
    double walker_dist[2] = {best_distance, best_distance};

    for (;;) {
        int side = walker_dx[1] < walker_dx[0];

        if (!(walker_dx[1] < best_distance || walker_dx[0] < best_distance))
            break;

        walker[side] += side ? -2 : 2;
        walker_dist[side] =
            distance_squared(x, y, walker[side], &walker_dx[side]);

        if (walker_dist[side] < best_distance) {
            best_distance = walker_dist[side];
            best = walker[side];
        }
    }
    return best;
}

/*
 * Anti-aliased color of the one-unit pixel at (x, y): the average color of
 * 4x4 sample points inside it.
 */
unsigned char *pixel_color(double x, double y)
{
    int red = 0, green = 0, blue = 0;
    int sample;

    for (sample = 0; sample < 16; ++sample) {
        double *site = nearest_site(x + sample / 4 / 4., y + sample % 4 / 4.);
        unsigned char *color = site_color(site);

        red += color[0];
        green += color[1];
        blue += color[2];
    }
    averaged_color[0] = red / 16;
    averaged_color[1] = green / 16;
    averaged_color[2] = blue / 16;
    return averaged_color;
}

/*
 * Draw the square of the given size whose corner is at (x, y).  If all four
 * corners have the same nearest site, the whole square gets that site's
 * color.  Otherwise it is split into four quarters, down to single pixels,
 * which are drawn with an anti-aliased color.
 */
void draw_square(double size, double x, double y)
{
    unsigned char *color;
    int quarter;

    if (size > 1) {
        double *site = nearest_site(x, y);

        if (site != nearest_site(x, y + size) ||
            site != nearest_site(x + size, y) ||
            site != nearest_site(x + size, y + size)) {
            size /= 2;
            for (quarter = 0; quarter < 4; ++quarter)
                draw_square(size, x + quarter / 2 * size,
                            y + quarter % 2 * size);
            return;
        }
        color = site_color(site);
    } else {
        color = pixel_color(x, y);
    }

    printf("<rect style=\"fill:rgb(%d,%d,%d        );\" x=\"%f\" y=\"%f\" "
           "width=\"%f\" height=\"%f\"/>\n",
           color[0], color[1], color[2], MARGIN + x, MARGIN + y, size, size);
}

int main()
{
    double *read_ptr, *input_copy;
    long index, count;
    int status;

    sites = calloc(WORKSPACE_SIZE, sizeof(double));
    input_copy = sites + WORKSPACE_SIZE;

    /* read every number, keeping a reversed copy at the top of the buffer */
    read_ptr = sites;
    while ((status = scanf("%lf", read_ptr)) > 0)
        *--input_copy = *read_ptr++;
    count = read_ptr - sites;

    /* bad input: print the Mersenne prime 2^25964951 - 1 in hexadecimal */
    if (status == 0 || count % 2 != 0 || count == 0) {
        long digits = 0x630C65;

        printf("\n0x7");
        while (digits-- > 0)
            printf("F");
        return -1;
    }

    /* sentinel sites far left and far right, then sort all sites by x */
    sites[count] = -WORKSPACE_SIZE;
    sites[count + 1] = -WORKSPACE_SIZE;
    sites[count + 2] = WORKSPACE_SIZE;
    sites[count + 3] = WORKSPACE_SIZE;
    site_count = count / 2;
    qsort(sites, site_count + 2, 2 * sizeof(double), compare_x);

    printf("<?xml version=\"1.0\"?>\n<svg width=\"%d\" height=\"%d\">\n",
           2 * MARGIN + GRID, 2 * MARGIN + GRID);
    draw_square(GRID, 0, 0);

    /* label each site with its input number: a black shadow, then ivory */
    read_ptr = sites + WORKSPACE_SIZE - 1;
    for (index = 1; index <= site_count; ++index, read_ptr -= 2) {
        double x = read_ptr[0], y = read_ptr[-1];
        int shade;

        for (shade = 1; shade >= 0; --shade) {
            printf("<circle style=\"fill:%s;\" cx=\"%f\" cy=\"%f\" r=\"3\"/>\n",
                   palette[shade], MARGIN + shade / 4. + GRID * x,
                   MARGIN + shade / 4. + GRID * y);
            printf("<text x=\"%f\" y=\"%f\" fill=\"%s\" text-anchor=\"end\">"
                   "%ld</text>\n",
                   MARGIN + shade / 2. - 5 + GRID * x,
                   MARGIN + shade / 2. - 5 + GRID * y, palette[shade], index);
        }
    }

    free(sites);
    printf("</svg>\n");
    return 0;
}
