#include <stdio.h>
#include <math.h>

/*
 * Sphere tracer that renders a line of text, written with a vector font made
 * of line segments and circular arcs, as glossy tubes floating above a
 * checkered floor.  The result is written to stdout as a 600x220 binary PPM.
 */

#define WIDTH 600
#define HEIGHT 220
#define MAX_SEGMENTS 50
#define MAX_TEXT_WIDTH 24   /* stop laying out text once this wide */
#define TUBE_RADIUS .45     /* thickness of the letters and floor offset */

typedef struct {
  float x, y, z;
} Vec;

static const Vec WHITE = {1, 1, 1};
static const Vec ZERO = {0, 0, 0};

/* The "dim" factor used for both lighting and reflectivity */
static const float DIM = .66;

Vec vec(float x, float y, float z) {
  Vec v;
  v.x = x;
  v.y = y;
  v.z = z;
  return v;
}

/* multiply and add: a + b * s */
Vec madd(Vec a, Vec b, float s) {
  return vec(a.x + b.x * s, a.y + b.y * s, a.z + b.z * s);
}

Vec sub(Vec a, Vec b) { return madd(a, b, -1); }

float dot(Vec a, Vec b) { return a.z * b.z + a.x * b.x + a.y * b.y; }

Vec normalize(Vec v) { return madd(ZERO, v, pow(dot(v, v), -.5)); }

float clamp01(float v) { return v < 0 ? 0 : v > 1 ? 1 : v; }

/*
 * Vector font.  Each stroke starts at (x, y) relative to the pen position.
 * LINE strokes go from the start point by the offset (dx, dy).
 * ARC strokes are unit circles centred at the start point, beginning at angle
 * first_quadrant * 90 degrees and sweeping counter-clockwise for the given
 * number of quadrants.
 */
typedef struct {
  int is_arc, x, y;
  int a, b; /* LINE: dx, dy   ARC: first_quadrant, quadrants */
} Stroke;

#define LINE(x, y, dx, dy) {0, x, y, dx, dy}
#define ARC(x, y, first_quadrant, quadrants) {1, x, y, first_quadrant, quadrants}
#define MAX_STROKES 5

typedef struct {
  char ch;
  int width, num_strokes;
  Stroke strokes[MAX_STROKES];
} Glyph;

static const Glyph font[] = {
    {' ', 1, 0, {}},
    {'!', 1, 2, {LINE(0, 0,  0, 0), LINE(0, 1,  0, 4)}},
    {'.', 1, 1, {LINE(0, 0,  0, 0)}},
    {'0', 3, 4, {LINE(0, 1,  0, 3), ARC(1, 1, -2, 2), ARC(1, 4,  0, 2), LINE(2, 1,  0, 3)}},
    {'1', 2, 2, {LINE(0, 4,  1, 1), LINE(1, 0,  0, 5)}},
    {'2', 3, 3, {LINE(0, 0,  2, 0), LINE(0, 0,  2, 4), ARC(1, 4,  0, 2)}},
    {'3', 3, 5, {LINE(0, 5,  2, 0), ARC(1, 1, -2, 2), ARC(1, 2,  0, 1), LINE(1, 3,  1, 2), LINE(2, 1,  0, 1)}},
    {'4', 3, 3, {LINE(0, 2,  1, 3), LINE(0, 2,  2, 0), LINE(2, 0,  0, 5)}},
    {'5', 3, 5, {LINE(0, 2,  0, 3), LINE(0, 5,  2, 0), ARC(1, 1, -2, 2), ARC(1, 2,  0, 2), LINE(2, 1,  0, 1)}},
    {'6', 3, 5, {LINE(0, 1,  0, 3), ARC(1, 1, -2, 2), ARC(1, 2,  0, 2), ARC(1, 4,  0, 2), LINE(2, 1,  0, 1)}},
    {'7', 3, 2, {LINE(0, 0,  2, 5), LINE(0, 5,  2, 0)}},
    {'8', 3, 5, {LINE(0, 1,  0, 1), ARC(1, 1, -2, 2), ARC(1, 2,  0, 2), ARC(1, 4, -2, 4), LINE(2, 1,  0, 1)}},
    {'9', 3, 5, {LINE(0, 3,  0, 1), ARC(1, 1, -2, 2), ARC(1, 3, -2, 2), ARC(1, 4,  0, 2), LINE(2, 1,  0, 3)}},
    {'a', 3, 4, {LINE(0, 3,  1, 0), ARC(1, 1, -2, 4), ARC(1, 2,  0, 1), LINE(2, 0,  0, 2)}},
    {'b', 3, 4, {LINE(0, 0,  0, 5), ARC(1, 1, -2, 2), ARC(1, 2,  0, 2), LINE(2, 1,  0, 1)}},
    {'c', 3, 3, {LINE(0, 1,  0, 1), ARC(1, 1, -2, 2), ARC(1, 2,  0, 2)}},
    {'d', 3, 4, {LINE(0, 1,  0, 1), ARC(1, 1, -2, 2), ARC(1, 2,  0, 2), LINE(2, 0,  0, 5)}},
    {'e', 3, 4, {LINE(0, 1,  0, 1), LINE(0, 2,  2, 0), ARC(1, 1, -2, 2), ARC(1, 2,  0, 2)}},
    {'f', 3, 3, {LINE(0, 3,  2, 0), LINE(1, 0,  0, 4), ARC(2, 4,  1, 1)}},
    {'g', 3, 4, {LINE(0, 0,  1, 0), ARC(1, 1, -1, 1), ARC(1, 2, -2, 4), LINE(2, 1,  0, 2)}},
    {'h', 3, 3, {LINE(0, 0,  0, 5), ARC(1, 2,  0, 2), LINE(2, 0,  0, 2)}},
    {'i', 1, 2, {LINE(0, 0,  0, 3), LINE(0, 4,  0, 0)}},
    {'j', 3, 3, {ARC(1, 1, -2, 2), LINE(2, 1,  0, 2), LINE(2, 4,  0, 0)}},
    {'k', 3, 3, {LINE(0, 0,  0, 5), LINE(0, 2,  2, 1), LINE(2, 0, -2, 2)}},
    {'l', 1, 1, {LINE(0, 0,  0, 5)}},
    {'m', 5, 5, {LINE(0, 0,  0, 3), ARC(1, 2,  0, 2), LINE(2, 0,  0, 2), ARC(3, 2,  0, 2), LINE(4, 0,  0, 2)}},
    {'n', 3, 3, {LINE(0, 0,  0, 3), ARC(1, 2,  0, 2), LINE(2, 0,  0, 2)}},
    {'o', 3, 4, {LINE(0, 1,  0, 1), ARC(1, 1, -2, 2), ARC(1, 2,  0, 2), LINE(2, 1,  0, 1)}},
    {'p', 3, 2, {LINE(0, 0,  0, 3), ARC(1, 2, -2, 4)}},
    {'q', 3, 2, {ARC(1, 2, -2, 4), LINE(2, 0,  0, 3)}},
    {'r', 3, 2, {LINE(0, 0,  0, 3), ARC(1, 2,  0, 2)}},
    {'s', 3, 3, {ARC(1, 1, -2, 2), ARC(1, 2,  0, 2), LINE(2, 1, -2, 1)}},
    {'t', 3, 3, {LINE(0, 3,  2, 0), LINE(1, 1,  0, 4), ARC(2, 1, -2, 1)}},
    {'u', 3, 3, {LINE(0, 1,  0, 2), ARC(1, 1, -2, 2), LINE(2, 0,  0, 3)}},
    {'v', 3, 2, {LINE(1, 0,  1, 3), LINE(1, 0, -1, 3)}},
    {'w', 5, 4, {LINE(1, 0,  1, 2), LINE(1, 0, -1, 3), LINE(3, 0,  1, 3), LINE(3, 0, -1, 2)}},
    {'x', 3, 2, {LINE(0, 0,  2, 3), LINE(2, 0, -2, 3)}},
    {'y', 3, 5, {LINE(0, 0,  1, 0), LINE(0, 2,  0, 1), ARC(1, 1, -1, 1), ARC(1, 2, -2, 2), LINE(2, 1,  0, 2)}},
    {'z', 3, 3, {LINE(0, 0,  2, 0), LINE(0, 0,  2, 3), LINE(0, 3,  2, 0)}},
};

/* A stroke placed in the scene */
typedef struct {
  Vec start; /* line start point or arc centre */
  Vec dir;   /* line direction and length (lines only) */
  int is_arc, first_quadrant, quadrants;
} Segment;

Segment segments[MAX_SEGMENTS];
int num_segments = 0;
float text_width = 0;

/* Results of the last call to scene_distance() */
Vec query_point, closest_point, surface_color;
float best_dist2;

/*
 * Offer candidate as the point nearest to query_point.  Returns nonzero if
 * it is closer than any point seen so far (the new squared distance).
 */
float consider(Vec candidate) {
  Vec diff = sub(query_point, candidate);
  float dist2 = dot(diff, diff);
  if (dist2 < best_dist2) {
    closest_point = candidate;
    best_dist2 = dist2;
    return best_dist2;
  }
  return 0;
}

/*
 * Distance from p to the nearest surface in the scene.  Also sets
 * closest_point and surface_color for the nearest object.
 */
float scene_distance(Vec p) {
  const float quarter_turn = M_PI * .5;
  int i;

  best_dist2 = 40;
  query_point = p;

  for (i = 0; i < num_segments; i++) {
    Vec start = segments[i].start, dir = segments[i].dir, nearest;

    /* centre the text horizontally */
    start.x += 8.8 - text_width * .45;

    if (segments[i].is_arc) {
      int first = segments[i].first_quadrant;
      int count = segments[i].quadrants;
      int last = first + count;
      float angle = atan2(p.y - start.y, p.x - start.x) / quarter_turn;

      /* clamp the angle to the arc, snapping to the nearer end */
      angle = quarter_turn * (angle > last + .5f * count ? first
                              : angle > last           ? last
                              : angle < first - .5f * count ? last
                              : angle < first          ? first
                                                       : angle);
      nearest = madd(start, vec(cos(angle), sin(angle), 0), 1);
    } else {
      /* nearest point on the line segment */
      nearest = madd(start, dir, clamp01(dot(sub(p, start), dir) / dot(dir, dir)));
    }
    consider(nearest);
  }

  /* floor below the text with a red and white checkerboard */
  {
    Vec floor_point = p;
    int checker;

    floor_point.y = -.9;
    checker = (int)(p.x / 8 + 8) ^ (int)(p.z / 8 + 8);
    surface_color = consider(floor_point)
                        ? (checker & 1 ? vec(DIM, 0, 0) : WHITE)
                        : vec(DIM, DIM, 1);
  }
  return sqrt(best_dist2) - TUBE_RADIUS;
}

/* Add the strokes of the glyph for character ch at the current pen position */
void add_glyph(int ch) {
  unsigned g;
  int s;

  for (g = 0; g < sizeof font / sizeof font[0]; g++) {
    if (font[g].ch != ch)
      continue;
    for (s = 0; s < font[g].num_strokes && num_segments < MAX_SEGMENTS; s++) {
      const Stroke *st = &font[g].strokes[s];
      Segment *seg = &segments[num_segments++];

      seg->start = vec(text_width + st->x, st->y, 0);
      seg->is_arc = st->is_arc;
      seg->first_quadrant = st->a;
      seg->quadrants = st->b;
      /* the tiny offset keeps a zero length line from dividing by zero */
      seg->dir = vec(st->a, st->b + 1e-4, 0);
    }
    text_width += font[g].width;
    return;
  }
}

/*
 * Follow one ray through up to bounces_left reflections and add the light it
 * gathers, scaled by weight, to *color.
 */
void trace(Vec *color, Vec pos, Vec dir, float weight, int bounces_left) {
  const Vec light = normalize(vec(-1, 1, 2));
  Vec normal, surface;
  float step, total, occlusion, sky, specular;
  int hit;

  do {
    /* sphere tracing: step forward by the distance to the nearest object */
    step = total = hit = 0;
    for (;;) {
      pos = madd(pos, dir, step);
      step = scene_distance(pos);
      total += step;
      if (!(total < 94))
        break;
      hit = step < .01;
      if (hit)
        break;
    }

    normal = normalize(sub(pos, closest_point));
    sky = dir.z * dir.z;
    surface = surface_color;

    /* ambient occlusion: sample the distance field along the normal */
    occlusion = 1;
    for (step += 1; step < 6 * hit; step += 1)
      occlusion -= clamp01(step / 3 - scene_distance(madd(pos, normal, step / 3))) /
                   pow(2, step);

    if (hit) {
      specular = pow(clamp01(dot(normal, normalize(sub(light, dir)))), 40);
      *color = madd(*color, WHITE, weight * specular);
      occlusion *= clamp01(dot(normal, light)) * .5f * DIM + DIM;
      /* keep some energy for the reflection on all but the last bounce */
      weight *= bounces_left-- ? DIM - DIM * specular : 1 - specular;
    } else {
      surface = vec(sky, sky, 1);
    }
    *color = madd(*color, surface, weight * occlusion);

    /* reflect the ray off the surface */
    pos = madd(pos, normal, .1);
    dir = madd(dir, normal, -2 * dot(dir, normal));
    weight *= .5;
  } while (hit && bounces_left >= 0);
}

int main(int argc, char **argv) {
  const char *text = argc > 1 ? argv[1] : "ioccc 2011";
  /* -preview (or any extra argument) renders one sample and no reflections */
  int quality = argc < 3 ? 3 : 0;
  int row, col, sample;

  /* the original printed the header while scanning the font for the first
     character, so an empty string gets no header */
  if (*text)
    printf("P6 %d %d 255\n", WIDTH, HEIGHT);

  for (; *text && text_width < MAX_TEXT_WIDTH; text++)
    add_glyph(*text | 32); /* letters are made lowercase */

  for (row = -HEIGHT / 2; row < HEIGHT / 2; row++) {
    for (col = -WIDTH / 2; col < WIDTH / 2; col++) {
      Vec pixel = ZERO;

      /* quality 3 gives 2x2 supersampling */
      for (sample = 0; sample <= quality; sample++) {
        Vec camera = vec(-4, 4.6, 29);
        Vec dir = normalize(
            madd(madd(madd(ZERO, normalize(vec(5, 0, 2)), col + col + sample / 2),
                      normalize(vec(2, -73, 0)), row + row + sample % 2),
                 vec(30.75, -6, -75), 20));
        trace(&pixel, camera, dir, 255 - quality * 64, quality);
      }
      putchar(pixel.x);
      putchar(pixel.y);
      putchar(pixel.z);
    }
  }
  return 0;
}
