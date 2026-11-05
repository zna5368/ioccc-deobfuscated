#include <stdio.h>

/* The region of the complex plane to draw. Change these to explore the fractal. */
static const double REAL_MIN = -2;
static const double REAL_MAX = 1;
static const double IMAGINARY_MIN = -1.3;
static const double IMAGINARY_MAX = 1.3;

/* Rows and columns are numbered from 0 up to and including these values. */
#define LAST_ROW 50
#define LAST_COLUMN 80

#define MAX_ITERATIONS 255
#define ESCAPE_LIMIT 8      /* compared against the squared distance from 0 */

/* The character for a point is picked by its iteration count, wrapping around. */
static const char shades[] = " +@-.";
#define SHADE_COUNT 5

/* Count how many steps of z = z * z + c it takes for z to escape. */
static int count_iterations(double c_real, double c_imaginary)
{
    double z_real = 0;
    double z_imaginary = 0;
    double real_squared;
    double imaginary_squared;
    int iterations = 0;

    do {
        real_squared = z_real * z_real;
        imaginary_squared = z_imaginary * z_imaginary;
        z_imaginary = 2 * z_real * z_imaginary + c_imaginary;
        z_real = real_squared - imaginary_squared + c_real;
        iterations++;
    } while (iterations < MAX_ITERATIONS
             && real_squared + imaginary_squared < ESCAPE_LIMIT);

    return iterations;
}

int main(void)
{
    for (int row = 0; row <= LAST_ROW; row++) {
        for (int column = 0; column <= LAST_COLUMN; column++) {
            double c_real = (double)column / LAST_COLUMN
                          * (REAL_MAX - REAL_MIN) + REAL_MIN;
            double c_imaginary = (double)row / LAST_ROW
                               * (IMAGINARY_MAX - IMAGINARY_MIN) + IMAGINARY_MIN;

            putchar(shades[count_iterations(c_real, c_imaginary) % SHADE_COUNT]);
        }
        putchar('\n');
    }
    return 0;
}
