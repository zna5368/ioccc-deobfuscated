/*
 * 2024/tmarrec - 3D smoke/fluid simulation rendered with ray marching
 *
 * stdio.h is pulled in by the Makefile with -include stdio.h
 *
 * NOTE: the Makefile builds with -O3 -ffast-math, which allows the compiler
 * to reorder floating point math and to use approximate division in
 * vectorized loops.  To keep the animation bit for bit equal to the original,
 * the loops use one flat counter split into x, y, z like the original does,
 * cols and field_size stay (non constant) variables, and advect() is not
 * inlined.  These keep the compiler making the same choices as for prog.c.
 */

/* simulation grid: N^3 interior cells surrounded by a one cell border */
#define N          80                   /* interior cells per axis */
#define SIDE       (N + 2)              /* cells per axis including border */
#define CELLS      (SIDE * SIDE * SIDE) /* cells in one field (551368) */
#define SLICE      (N * N)              /* interior cells in one z slice */
#define INTERIOR   (N * N * N)          /* interior cells in one field */
#define HALF       40                   /* half of the grid (box half size) */

/* screen: ROWS x COLS character cells, each one a colored character */
#define ROWS       64
#define COLS       79
#define CELL_CHARS 12                       /* "\033[38;5;1XXm" + 1 char */
#define ROW_CHARS  (COLS * CELL_CHARS + 1)  /* plus the newline (949) */
#define PIXELS     (ROWS * COLS)            /* 5056 */
#define RAYS_SIZE  (PIXELS * 3)             /* xyz ray direction per pixel */

#define SQRT_MAGIC 0x1fbb4000   /* bit trick constant for a fast sqrt */

/* the fields stored after the ray directions */
enum {
    VEL_X, VEL_Y, VEL_Z,        /* velocity (VEL_Y is also pressure scratch) */
    OLD_X, OLD_Y, OLD_Z,        /* force, then the previous velocity */
    DENSITY,                    /* smoke density */
    OLD_DENSITY                 /* smoke source, then the previous density */
};

int cols = COLS;                /* screen columns */
int field_size = CELLS;         /* floats in one field */
float camX = 240, camZ;         /* camera position, circling the box */
float step = .53;               /* ray step through the smoke */
float world[RAYS_SIZE + 8 * CELLS];       /* ray directions then fields */
char screen[ROWS * ROW_CHARS + 1];

/* density ramp from empty to thick smoke */
char ramp[] = "             (\\tjxucXUCQOmqdkaoM&%$";
/* default screen cell: color 141 and a '+' for the background */
char cell_template[] = "\033[38;5;141m+";

/* address of a cell in a field (coordinates include the border) */
float *cell(int x, int y, int z, int field)
{
    return &world[(x + SIDE * y + SIDE * SIDE * z) + (RAYS_SIZE + field * field_size)];
}

/* sqrt(x*x + y*y + z*z) approximated by halving the float exponent bits */
float approx_length(float x, float y, float z)
{
    union { float f; int i; } v;

    v.f = x * x + y * y + z * z;
    v.i = SQRT_MAGIC + (v.i >> 1);
    return v.f;
}

void normalize(float x, float y, float z, float *ox, float *oy, float *oz)
{
    float len = approx_length(x, y, z);

    *ox = x / len;
    *oy = y / len;
    *oz = z / len;
}

/* the camera space ray direction stored for pixel number p */
float *ray(int p)
{
    return &world[p * 3];
}

float absf(float v)
{
    return 0 > v ? -v : v;
}

void clamp(float *v)
{
    *v = *v < .5 ? .5 : *v > 85 ? 85 : *v;
}

/* dst += 0.4 * src over the whole field */
void add_source(int dst, int src)
{
    int i;

    for (i = 0; i < field_size; ++i)
        world[RAYS_SIZE + dst * field_size + i] += .4 * world[RAYS_SIZE + src * field_size + i];
}

/* copy the INTERIOR of field src into field dst */
void copy_field(int dst, int src)
{
    int n, x, y, z;

    for (n = 0; n < INTERIOR; ++n) {
        x = n % SLICE % N + 1;
        y = n % SLICE / N + 1;
        z = n / SLICE + 1;
        *cell(x, y, z, dst) = *cell(x, y, z, src);
    }
}

/*
 * semi-lagrangian advection: trace each cell backwards along the velocity
 * (u, v, w) and trilinearly interpolate field src at that spot into dst
 */
__attribute__((noinline))
void advect(int dst, int src, int u, int v, int w)
{
    int n, x, y, z;

    for (n = 0; n < INTERIOR; ++n) {
        x = n % SLICE % N + 1;
        y = n % SLICE / N + 1;
        z = n / SLICE + 1;

        float px = x - N * .4 * *cell(x, y, z, u);
        float py = y - N * .4 * *cell(x, y, z, v);
        float pz = z - N * .4 * *cell(x, y, z, w);

        clamp(&px);
        clamp(&py);
        clamp(&pz);

        float fx = px - (int)px;
        float fy = py - (int)py, gy = 1 - fy;
        float fz = pz - (int)pz, gz = 1 - fz;

        *cell(x, y, z, dst) =
            (1 - fx) * (gy * gz * *cell(px, py, pz, src) +
                        fy * gz * *cell(px, 1 + py, pz, src) +
                        gy * fz * *cell(px, py, pz + 1, src) +
                        fy * fz * *cell(px, 1 + py, 1 + pz, src)) +
            fx * (gy * gz * *cell(px + 1, py, pz, src) +
                  fy * gz * *cell(1 + px, 1 + py, pz, src) +
                  gy * fz * *cell(1 + px, py, 1 + pz, src) +
                  fy * fz * *cell(1 + px, py + 1, pz + 1, src));
    }
}

/* subtract the pressure gradient along one axis (dx, dy, dz) */
void subtract_gradient(int vel, int pressure, int x, int y, int z,
                       int dx, int dy, int dz)
{
    *cell(x, y, z, vel) -= 40 * (*cell(x + dx, y + dy, z + dz, pressure) -
                                 *cell(x - dx, y - dy, z - dz, pressure));
}

/* make the velocity (u, v, w) divergence free using a pressure field */
void project(int u, int v, int w, int pressure)
{
    int n, x, y, z;

    for (n = 0; n < INTERIOR; ++n) {
        x = n % SLICE % N + 1;
        y = n % SLICE / N + 1;
        z = n / SLICE + 1;
        *cell(x, y, z, pressure) = -1. / 3 *
            ((*cell(1 + x, y, z, u) - *cell(x - 1, y, z, u)) / N +
             (*cell(x, y + 1, z, v) - *cell(x, y - 1, z, v)) / N +
             (*cell(x, y, 1 + z, w) - *cell(x, y, z - 1, w)) / N);
    }

    for (n = 0; n < INTERIOR; ++n) {
        x = n % SLICE % N + 1;
        y = n % SLICE / N + 1;
        z = n / SLICE + 1;
        subtract_gradient(u, pressure, x, y, z, 1, 0, 0);
        subtract_gradient(v, pressure, x, y, z, 0, 1, 0);
        subtract_gradient(w, pressure, x, y, z, 0, 0, 1);
    }
}

int main()
{
    float phase = 0;    /* angle that makes the smoke jet wobble */
    int row, col, n, x, y, z, i;

    /*
     * end every screen row with a newline and aim a ray through each pixel,
     * the direction of pixel p is stored in slot p + 1 so the picture is
     * shifted right by one column
     */
    for (row = 0; row < ROWS; ++row) {
        screen[row * ROW_CHARS + cols * CELL_CHARS] = '\n';
        for (col = 0; col < cols; ++col) {
            float dx, dy, dz;
            int p = (row * cols + col + 1) * 3;

            normalize(col - 40, row - 32, -145, &dx, &dy, &dz);
            world[p] = dx;
            world[p + 1] = dy;
            world[p + 2] = dz;
        }
    }

    for (;;) {
        float fwdX, fwdY, fwdZ, rightX, rightY, rightZ, next;

        fputs("\033[H", stdout);    /* cursor to the top left corner */

        /* clear the forces and the smoke source (i is a flat cell index) */
        for (i = 0; i < field_size; ++i)
            *cell(i, 0, 0, OLD_X) = *cell(i, 0, 0, OLD_Y) =
                *cell(i, 0, 0, OLD_Z) = *cell(i, 0, 0, OLD_DENSITY) = 0;

        /* a wobbling jet near the bottom and smoke at the floor */
        for (n = 0; n < INTERIOR; ++n) {
            x = n % SLICE % N + 1;
            y = n % SLICE / N + 1;
            z = n / SLICE + 1;

            float r = approx_length(x - HALF, z - HALF, 0);

            if (y > 10 && y < 13 && r < 2) {
                float angle = phase, sq = angle * angle, sine, cosine;

                /* taylor series of sin and cos */
                sine = angle - (sq * angle) / 6 + (sq * sq * angle) / 120;
                cosine = 1 - sq / 2 + (sq * sq) / 24;
                *cell(x, y, z, OLD_Y) = 1.5;
                *cell(x, y, z, OLD_X) = sine / 2 + .0275;
                *cell(x, y, z, OLD_Z) = cosine * .7 - .35;
            }
            if (y < 2 && r < 3)
                *cell(x, y, z, OLD_DENSITY) = .1;
        }

        /* velocity step */
        add_source(VEL_X, OLD_X);
        add_source(VEL_Y, OLD_Y);
        add_source(VEL_Z, OLD_Z);
        copy_field(OLD_Z, VEL_Z);
        copy_field(OLD_Y, VEL_Y);
        copy_field(OLD_X, VEL_X);
        project(OLD_X, OLD_Y, OLD_Z, VEL_Y);
        advect(VEL_Y, OLD_Y, OLD_X, OLD_Y, OLD_Z);
        advect(VEL_X, OLD_X, OLD_X, OLD_Y, OLD_Z);
        advect(VEL_Z, OLD_Z, OLD_X, OLD_Y, OLD_Z);
        project(VEL_X, VEL_Y, VEL_Z, OLD_Y);

        /* density step */
        add_source(DENSITY, OLD_DENSITY);
        copy_field(OLD_DENSITY, DENSITY);
        advect(DENSITY, OLD_DENSITY, VEL_X, VEL_Y, VEL_Z);

        /* orbit the camera around the box and look at the center */
        next = camX * .9998 + camZ * .02;
        camZ = .9998 * camZ - camX * .02;
        camX = next;
        normalize(-camX, 0, -camZ, &fwdX, &fwdY, &fwdZ);
        phase += .1;
        phase = phase > 3.14 ? -phase : phase;
        normalize(fwdZ, 0, -fwdX, &rightX, &rightY, &rightZ);

        /* ray march the box and the smoke inside of it */
        for (i = 0; i < PIXELS; ++i) {
            int k, shade, color;
            float dist = 0, *d;
            char *out;

            row = i % ROWS;
            col = i / ROWS;
            d = ray(row * cols + col);
            /* rotate the camera space ray into the world */
            float dirX = d[0] * rightX + d[1] * (rightY * fwdZ - fwdY * rightZ) + d[2] * -fwdX;
            float dirY = d[0] * rightY + d[1] * (rightZ * fwdX - fwdZ * rightX) + d[2] * -fwdY;
            float dirZ = d[0] * rightZ + d[1] * (rightX * fwdY - fwdX * rightY) + d[2] * -fwdZ;

            out = &screen[row * ROW_CHARS + col * CELL_CHARS];
            for (k = 0; k < CELL_CHARS; ++k)
                out[k] = cell_template[k];

            while (dist < N * 5) {
                /* signed distance to a box of HALF N 40 at the origin */
                float bx = absf(camX + dirX * dist) - HALF;
                float by = absf(dirY * dist) - HALF;
                float bz = absf(camZ + dirZ * dist) - HALF;
                float byz = by > bz ? by : bz, bmax = bx > byz ? bx : byz;
                float sdf = (bmax < 0 ? bmax : 0) +
                    approx_length(bx < 0 ? 0 : bx, by < 0 ? 0 : by, bz > 0 ? bz : 0);

                if (sdf < .01) {
                    /* hit the box: walk through the smoke in grid coordinates */
                    float gx = camX + dist * dirX + HALF;
                    float gy = dist * dirY + HALF;
                    float gz = camZ + dist * dirZ + HALF;
                    float transmit = 1;

                    do {
                        transmit *= 1 - *cell(gz + 1, 1 + gy, gx + 1, DENSITY) * step;
                        gx = gx + dirX * step;
                        gy = gy + dirY * step;
                        gz = gz + dirZ * step;
                    } while (N > gx & gy < N & gz < N & gx > 0 & 0 < gy & gz > 0);

                    shade = (1 - transmit) * 35;
                    shade = 0 > shade ? 0 : shade > 34 ? 34 : shade;
                    color = 240 + .44 * shade;
                    /* replace the last two digits of the color 141 */
                    out[8] = (color - 200) / 10 + '0';
                    out[9] = color % 10 + '0';
                    out[11] = ramp[shade];
                    break;
                }
                dist += sdf;
            }
        }
        puts(screen);
    }
}
