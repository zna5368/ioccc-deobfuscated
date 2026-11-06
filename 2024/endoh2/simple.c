#include <stdio.h>
#include <unistd.h>

/*
 * Simulation parameters, supplied on the compiler command line:
 *
 *   -DW=120 -DH=24 -Ddt=0.01 -De=0.5 -Du=0.5 -Drpm=750 -Dtilt=5
 *
 * Because "e" and "u" are macros, no identifier in this file may be
 * named e or u (or W, H, dt, rpm, tilt).
 */
#define SCREEN_WIDTH   W
#define SCREEN_HEIGHT  H
#define TIME_STEP      dt
#define RESTITUTION    e
#define FRICTION       u
#define INITIAL_RPM    rpm
#define INITIAL_TILT   tilt

#define MAX_PARTICLES  (1 << 20)
#define SUBSTEPS       20     /* physics steps per displayed frame        */
#define TAYLOR_TERMS   99     /* terms used for sin/cos series            */
#define SQRT_ITERS     99     /* Newton iterations used for square roots  */

/*
 * A quaternion (x, y, z, w) where w is the real part.  A 3D vector is a
 * quaternion whose real part is zero.
 *
 * World axes:  x = up (height above the floor)
 *              y = horizontal, left/right on the screen
 *              z = depth (towards/away from the viewer)
 */
typedef struct Quat {
    float x, y, z, w;
} Quat;

/* A point mass of the rigid body */
typedef struct Particle {
    Quat body_pos;    /* position relative to the center of mass (body frame) */
    Quat world_pos;   /* position in the world, updated every substep         */
    float mass;
} Particle;

static Particle particles[MAX_PARTICLES];
static int particle_count;

/* screen: cursor-home + clear-screen escape, followed by the picture */
static char screen_buf[SCREEN_HEIGHT * SCREEN_WIDTH + 9] = "\x1b[H\x1b[J";
static char *screen = screen_buf + 6;

static const Quat ZERO = {0, 0, 0, 0};
static const Quat UP   = {1, 0, 0, 0};

/* access the x, y or z component by number (0, 1, 2) */
static float *component(Quat *q, int axis)
{
    return axis == 0 ? &q->x : axis == 1 ? &q->y : &q->z;
}

static Quat add(Quat a, Quat b)
{
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    a.w += b.w;
    return a;
}

/* Hamilton product a*b.  For two vectors: w = -dot(a,b), xyz = cross(a,b) */
static Quat mul(Quat a, Quat b)
{
    Quat r;
    r.x = a.x * b.w + a.y * b.z - a.z * b.y + a.w * b.x;
    r.y = a.y * b.w - a.x * b.z + a.w * b.y + a.z * b.x;
    r.z = a.z * b.w + a.w * b.z + a.x * b.y - a.y * b.x;
    r.w = a.w * b.w - a.z * b.z - a.y * b.y - a.x * b.x;
    return r;
}

static Quat scale(Quat a, float factor)
{
    a.x *= factor;
    a.y *= factor;
    a.z *= factor;
    a.w *= factor;
    return a;
}

static float dot(Quat a, Quat b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

/* Square root by Newton's method (math.h is not used) */
static float square_root(float value)
{
    float root = SQRT_ITERS;
    int iter;

    for (iter = 0; iter < SQRT_ITERS; iter++)
        root = (value / root + root) / 2;
    return root;
}

static Quat normalize(Quat a)
{
    float inv_len = 1 / square_root(dot(a, a));
    return scale(a, inv_len);
}

/*
 * Subtract radius*sin(angle) from *sin_out and radius*cos(angle) from
 * *cos_out, using the Taylor series of sin and cos.
 */
static void sub_sin_cos(float *sin_out, float *cos_out, float radius, float angle)
{
    float term = radius;
    int n;

    for (n = 1; n < TAYLOR_TERMS; n++) {
        term = n % 2 ? -term : term;
        if (n % 2)
            *cos_out += term;
        else
            *sin_out += term;
        term *= angle / n;
    }
}

/* Linear congruential generator for jittering particle radii */
static int next_random(int seed)
{
    return (int)(48271u * (unsigned)seed) % 65535;
}

/*
 * Read the shape from stdin.  Every visible character becomes a ring of
 * particles: the picture is spun around its left edge.  Characters with an
 * odd code are heavy, even ones are light, a '/' starts a comment.
 * Returns the (negated) total mass.
 */
static float read_body(void)
{
    static char line[SCREEN_WIDTH * SCREEN_HEIGHT - 1];
    Quat mass_moment = ZERO;
    float neg_total_mass = 0;
    int row = 0, col, ch, seed = 1, i;

    while (fgets(line, sizeof line, stdin)) {
        for (col = 1; (ch = line[col - 1]) && ch != '/'; col++) {
            float angle = 0, step;

            /* walk once around a circle of radius ~col/2 */
            while (ch > ' ' && angle * 113 < 710) {      /* 710/113 ~= 2*pi */
                Particle *p = &particles[particle_count++];
                float radius;

                angle += step = 2. / col;

                /* columns are half as wide as rows are tall */
                radius = (col - .5 + seed * 61e-7) / 2;
                step /= ch % 2 ? 2 : 2e3;
                neg_total_mass -= step;

                p->body_pos.x = row;
                sub_sin_cos(&p->body_pos.y, &p->body_pos.z, radius, angle);
                p->mass = step;
                mass_moment = add(mass_moment, scale(p->body_pos, p->mass));

                seed = next_random(seed);
            }
        }
        row--;
    }

    /* move the origin to the center of mass */
    for (i = particle_count; i--;)
        particles[i].body_pos = add(particles[i].body_pos,
                                    scale(mass_moment, 1 / neg_total_mass));
    return neg_total_mass;
}

/*
 * Inverse of the inertia tensor, one row per quaternion (real part 0).
 * inv[3] stays zero.
 */
static void inverse_inertia(Quat inv[4])
{
    Quat neg_inertia[3] = {ZERO, ZERO, ZERO};   /* rows of minus the tensor */
    float neg_triple_det = 0;
    int n, i;

    for (n = 0; n < particle_count; n++) {
        Quat r = particles[n].body_pos;
        float m = particles[n].mass;

        for (i = 3; i--;) {
            neg_inertia[i] = add(neg_inertia[i], scale(r, *component(&r, i) * m));
            *component(&neg_inertia[i], i) += mul(r, r).w * m;   /* -|r|^2 */
        }
    }

    /* inverse = cofactor rows / determinant, cofactors are cross products */
    for (i = 0; i < 3; i++) {
        inv[i] = mul(neg_inertia[(i + 1) % 3], neg_inertia[(i + 2) % 3]);
        neg_triple_det -= dot(neg_inertia[i], inv[i]);
    }
    for (; i--;) {
        inv[i] = scale(inv[i], 3 / neg_triple_det);
        inv[i].w = 0;
    }
    inv[3] = ZERO;
}

int main()
{
    Quat inv_inertia[4];
    Quat orientation = ZERO;
    Quat omega = {71 * INITIAL_RPM / 13560., 0, 0, 0}; /* 71/13560 ~= pi/600 */
    Quat velocity = ZERO;
    Quat center = {30, 0, 0, 0};                       /* center of mass */
    float neg_mass, camera = 0;
    int i, step, n;

    /* tilt the body around the depth axis (half angle in radians, 71/8136 ~= pi/360) */
    sub_sin_cos(&orientation.z, &orientation.w, 1, 71. / 8136 * INITIAL_TILT);

    neg_mass = read_body();
    inverse_inertia(inv_inertia);

    for (;; usleep(1e4)) {
        /* blank screen with a scrolling floor on the last row */
        for (i = SCREEN_HEIGHT * SCREEN_WIDTH; i--;) {
            int floor_x = i + camera;

            if (i > SCREEN_HEIGHT * SCREEN_WIDTH - SCREEN_WIDTH)
                screen[i] = (floor_x & 16) ? '=' : '-';
            else
                screen[i] = i % SCREEN_WIDTH ? ' ' : '\n';
        }

        for (step = SUBSTEPS; step--;) {
            Quat force = {neg_mass, 0, 0, 0};   /* gravity */
            Quat torque = ZERO;
            Quat delta_omega = ZERO;
            float deepest = 0;

            /* draw the particle positions of the previous substep (motion blur) */
            for (n = 0; n < particle_count; n++) {
                Quat pos = particles[n].world_pos;
                int row = SCREEN_HEIGHT - 1.5 - pos.x;
                int col = .5 + (pos.y - camera) * 2 + SCREEN_WIDTH / 2;

                i = SCREEN_WIDTH * row + col;
                if (i > 0 && i < SCREEN_WIDTH * SCREEN_HEIGHT && 0 < col && col < SCREEN_WIDTH)
                    screen[i] = pos.z < center.z && screen[i] != '@' ? ':' : '@';
            }

            for (n = 0; n < particle_count; n++) {
                Particle *p = &particles[n];
                Quat conj = scale(orientation, -1);
                Quat r, response, contact_force;
                float height;

                conj.w *= -1;
                /* rotate the body-frame point into the world */
                r = mul(mul(orientation, p->body_pos), conj);
                p->world_pos = add(r, center);
                height = p->world_pos.x;

                if (height < 0) {
                    /* the particle is below the floor: compute an impulse */
                    Quat r_cross_up = mul(r, UP);
                    Quat point_vel, slide;
                    float impulse;

                    response.x = dot(inv_inertia[0], r_cross_up);
                    response.y = dot(inv_inertia[1], r_cross_up);
                    response.z = dot(inv_inertia[2], r_cross_up);
                    response.w = dot(inv_inertia[3], r_cross_up);

                    point_vel = add(velocity, mul(omega, r));
                    impulse = (1 + RESTITUTION) * point_vel.x
                              / (mul(response, r).x - 1 / neg_mass);

                    /* sliding velocity along the floor */
                    slide = point_vel;
                    slide.x = slide.w = 0;

                    contact_force = add(scale(UP, impulse * height - impulse),
                                        scale(normalize(slide), impulse * FRICTION));
                    force = add(force, contact_force);
                    torque = add(torque, mul(r, contact_force));
                    deepest = deepest > height ? height : deepest;
                    delta_omega = contact_force;   /* original reused one variable; only .w (always 0) survives */
                }
            }

            /* push the body out of the floor and integrate */
            center.x -= deepest;
            velocity = add(velocity, scale(force, TIME_STEP / -neg_mass));
            center = add(center, scale(velocity, TIME_STEP));

            delta_omega.x = dot(inv_inertia[0], torque) * TIME_STEP;
            delta_omega.y = dot(inv_inertia[1], torque) * TIME_STEP;
            delta_omega.z = dot(inv_inertia[2], torque) * TIME_STEP;
            omega = add(omega, delta_omega);
            orientation = normalize(add(orientation,
                                        scale(mul(omega, orientation), .5 * TIME_STEP)));
        }

        /* camera slowly follows the body */
        camera += (center.y - camera) / 50;

        /* omega = rpm * pi/600, so rpm = |omega| * 600/pi and 36476 ~= (600/pi)^2 */
        printf("%s\n%*.0f rpm", screen_buf, SCREEN_WIDTH - 5,
               square_root(dot(omega, omega) * 36476));
        screen_buf[5] = 'H';   /* after the first frame: home only, no clear */
    }
}
