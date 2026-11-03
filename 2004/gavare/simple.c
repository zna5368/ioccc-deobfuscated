/*
 * Fixed-point ray tracer that renders the letters "IOCCC ray" made of
 * reflective spheres above a ground plane, written as a binary PPM image.
 *
 * All arithmetic is integer: a value v is stored as v * SCALE.
 */

int width = 1024;
int height = 768;
int antialias = 3; /* antialias x antialias samples per pixel */

int camera_x = 0;
int camera_y = -10;
int camera_z = -7;

#define SCALE 1296
#define MAX_COLOR 255
#define MAX_DEPTH 9
#define NUM_SPHERES 44
#define SPHERE_RADIUS 2

/* Sphere centres (unscaled).  The first 22 spheres spell "IOCCC" standing
 * upright at y = 9, the remaining 22 spell "ray" lying flat at y = 0. */
int sphere_x[NUM_SPHERES] = {
    -15, -15, -15, -15,                /* I */
    -8,  -8,  -10, -6,  -10, -6,       /* O */
    -1,  -1,  1,   1,                  /* C */
    6,   6,   8,   8,                  /* C */
    13,  13,  15,  15,                 /* C */
    -11, -11, -11, -11, -9, -9, -7, -7, /* r */
    0,   0,   -2,  -2,  -2, 2,  2,  2,  /* a */
    7,   7,   9,   9,   11, 11,         /* y */
};
int sphere_z[NUM_SPHERES] = {
    3,   0,   -3,  -6,                       /* I */
    -6,  3,   0,   0,   -3,  -3,             /* O */
    0,   -3,  -6,  3,                        /* C */
    0,   -3,  -6,  3,                        /* C */
    0,   -3,  -6,  3,                        /* C */
    -11, -13, -15, -17, -11, -15, -13, -17,  /* r */
    -11, -15, -13, -15, -17, -13, -15, -17,  /* a */
    -11, -13, -15, -17, -11, -13,            /* y */
};

int sphere_center_y(int i) { return i < 22 ? 9 : 0; }

/* Colour mask of a sphere: bit 0 = red, bit 1 = green, bit 2 = blue. */
int sphere_color(int i) {
    if (i < 22)
        return 7; /* IOCCC: white */
    if (i < 30)
        return 1; /* r: red */
    if (i < 38)
        return 2; /* a: green */
    return 4;     /* y: blue */
}

/* Integer square root, found one bit at a time from the top bit down. */
int isqrt(int n) {
    int guess = 0, bit;
    for (bit = 1 << 15; bit; bit /= 2) {
        guess ^= bit;
        if (guess * guess > n)
            guess ^= bit;
    }
    return guess;
}

/* Distance along the (unit) ray to sphere i, or a value <= 0 on a miss. */
int hit_sphere(int i, int x, int y, int z, int dx, int dy, int dz) {
    int b, half_b, root;

    x -= sphere_x[i] * SCALE;
    y -= sphere_center_y(i) * SCALE;
    z -= sphere_z[i] * SCALE;

    /* Solve q^2 - 2*half_b*q + b = 0 */
    b = x * x / SCALE + y * y / SCALE + z * z / SCALE -
        SPHERE_RADIUS * SPHERE_RADIUS * SCALE;
    half_b = -x * dx / SCALE - y * dy / SCALE - z * dz / SCALE;

    b = half_b * half_b / SCALE - b; /* discriminant */
    if (b < 0)
        return -1;
    root = isqrt(b * SCALE);
    return half_b + (half_b > root ? -root : root);
}

int hit_index;    /* closest sphere hit, or -1 */
int hit_distance; /* distance to it */

/* Find the closest sphere hit by the ray, ignoring sphere `skip`. */
void find_closest(int x, int y, int z, int dx, int dy, int dz, int skip) {
    int i, q;
    hit_index = -1;
    for (i = 0; i < NUM_SPHERES; i++) {
        q = hit_sphere(i, x, y, z, dx, dy, dz);
        if (q > 0 && i != skip && (q < hit_distance || hit_index < 0)) {
            hit_distance = q;
            hit_index = i;
        }
    }
}

int clamp(int c) { return c < 0 ? 0 : c > MAX_COLOR ? MAX_COLOR : c; }

int red, green, blue; /* result of trace_ray */

void trace_ray(int x, int y, int z, int dx, int dy, int dz, int depth,
               int skip) {
    int nx, ny, nz, len, cosine, shade, hit, color, t;

    find_closest(x, y, z, dx, dy, dz, skip);

    if (depth > 0 && hit_index >= 0) {
        hit = hit_index;

        /* move to the hit point */
        x += dx * hit_distance / SCALE;
        y += dy * hit_distance / SCALE;
        z += dz * hit_distance / SCALE;

        /* surface normal (not yet normalized) */
        nx = x - sphere_x[hit] * SCALE;
        ny = y - sphere_center_y(hit) * SCALE;
        nz = z - sphere_z[hit] * SCALE;

        /* diffuse highlight from a light in direction (-2,-2,1)/3 */
        shade = (-2 * nx - 2 * ny + nz) / 3;
        len = isqrt(nx * nx + ny * ny + nz * nz);
        shade /= SPHERE_RADIUS;
        shade *= shade;
        shade *= 200;
        shade /= SCALE * SCALE;

        /* reflect the ray direction about the normal */
        if (len != 0) {
            nx = -nx * SCALE / len;
            ny = -ny * SCALE / len;
            nz = -nz * SCALE / len;
        }
        cosine = (dx * nx + dy * ny + dz * nz) / SCALE;
        dx -= nx * cosine / (SCALE / 2);
        dy -= ny * cosine / (SCALE / 2);
        dz -= nz * cosine / (SCALE / 2);

        trace_ray(x, y, z, dx, dy, dz, depth - 1, hit);

        /* half reflection plus the sphere's own tinted highlight */
        red /= 2;
        green /= 2;
        blue /= 2;
        color = sphere_color(hit);
        if (color & 1)
            red += shade;
        if (color & 2)
            green += shade;
        if (color & 4)
            blue += shade;
    } else {
        /* Missed everything: sky or ground.  Primary rays have no
         * vertical direction, so fake one from the ray origin height. */
        if (depth == MAX_DEPTH) {
            z += 2;
            dz = z > 0 ? z / 8 : z / 20;
        }
        if (dz > 0) {
            /* sky: white at the horizon, blue above */
            t = dz * dz / SCALE;
            red = 255 - 250 * t / SCALE;
            green = 255 - 150 * t / SCALE;
            blue = 255 - 100 * t / SCALE;
        } else {
            t = dz * dz / SCALE;
            if (t < SCALE / 5) {
                /* horizon fading into the ground */
                red = 255 - 210 * t / SCALE;
                green = 255 - 435 * t / SCALE;
                blue = 255 - 720 * t / SCALE;
            } else {
                /* ground: light brown to dark brown */
                t -= SCALE / 5;
                red = 213 - 110 * t / SCALE;
                green = 168 - 113 * t / SCALE;
                blue = 111 - 85 * t / SCALE;
            }
        }
        if (depth != MAX_DEPTH) {
            red /= 2;
            green /= 2;
            blue /= 2;
        }
    }

    red = clamp(red);
    green = clamp(green);
    blue = clamp(blue);
}

void render_pixel(int px, int py) {
    int r = 0, g = 0, b = 0, sx, sy;

    for (sy = 0; sy < antialias; sy++) {
        for (sx = 0; sx < antialias; sx++) {
            trace_ray(SCALE * camera_x +
                          SCALE * 40 * (antialias * px + sx) / width /
                              antialias -
                          SCALE * 20,
                      SCALE * camera_y,
                      SCALE * camera_z -
                          SCALE * 30 * (antialias * py + sy) / height /
                              antialias +
                          SCALE * 15,
                      0, SCALE, 0, MAX_DEPTH, -1);
            r += red;
            g += green;
            b += blue;
        }
    }
    printf("%c%c%c", r / antialias / antialias, g / antialias / antialias,
           b / antialias / antialias);
}

int main() {
    int x, y;

    printf("P6\n%i %i\n255\n", width, height);
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            render_pixel(x, y);
    return 0;
}
