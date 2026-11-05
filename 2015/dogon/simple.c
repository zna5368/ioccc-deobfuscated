#include <math.h>
#include <X11/Xlib.h>

/*
 * The window size comes from the Makefile: -DW=<width> -Dp=<height>.
 */
#define WINDOW_WIDTH W
#define WINDOW_HEIGHT p
#define PIXEL_COUNT (WINDOW_WIDTH * WINDOW_HEIGHT)

/*
 * The screen is visited in four interleaved passes per axis, so that every
 * fourth row and column is finished before the pixels between them.
 */
#define INTERLEAVE 4
#define COLUMN_STRIDE (WINDOW_WIDTH / INTERLEAVE + 1)
#define ROW_STRIDE (WINDOW_HEIGHT / INTERLEAVE + 1)

/*
 * A Packed value holds several small numbers side by side, so that one 64 bit
 * addition or multiplication works on all of them at once.
 *
 * Position or direction: three 20 bit lanes, x in bits 0-19, y in bits 20-39
 * and z in bits 40-59. Each lane is a fixed point number with 10 fraction bits,
 * then 6 bits of voxel coordinate.
 *
 * Colour: blue in bits 0-7, red in bits 16-23, green in bits 32-39. The gaps
 * give each channel room to be multiplied by a brightness.
 *
 * Texture coordinate: u in bits 0-9, v in bits 12-21, then the two face flags.
 *
 * Surface cache entry, one for each pixel:
 *   bits 0-1    axis of the face that was hit, or 3 if leaves are in the way
 *   bits 2-19   voxel index
 *   bits 32-55  texture coordinate
 *   bits 56-63  brightness
 */
typedef unsigned long long Packed;

#define LANE_BITS 20
#define VOXEL_SIZE 1024            /* one voxel in fixed point units */
#define FRACTION_MASK 1023

#define WORLD_SIZE_BITS 6
#define WORLD_SIZE 64              /* the world wraps around in each axis */
#define WORLD_CENTER 32
#define VOXEL_COUNT (1 << 18)

#define START_POSITION 0x820008202625a0    /* (2441.406, 32.5019, 32.5) */
#define FAR_DISTANCE 32768                 /* everything fades out by here */

/* The axis numbers, and the only axis with a top and bottom face. */
#define AXIS_COUNT 3
#define AXIS_Y 1

#define BLOCK_EMPTY 0
#define BLOCK_STONE 1
#define BLOCK_WATER 2
#define BLOCK_WOOD 3
#define BLOCK_GRASS 4
#define BLOCK_BRICK 5
#define BLOCK_LEAVES 8
#define BLOCK_TYPES 16

/* Each block type has three 16x16 faces, stacked as top, side and bottom. */
#define TEXTURE_SIZE 16
#define TEXTURE_ROWS 48
#define TEXELS_PER_BLOCK (TEXTURE_SIZE * TEXTURE_ROWS)
#define TEXTURE_TABLE_SIZE (1 << 14)

#define TEXTURE_V_MASK 0x3FF000
#define TEXTURE_SIDE_FLAG (1 << 22)
#define TEXTURE_FLIP_SHIFT 23
#define TEXTURE_CARRY_MASK 0xFFF3FF        /* drops the carry from u into v */

#define COLOUR_DIRT 0x966C4A
#define COLOUR_GRASS 0x6AAA40
#define COLOUR_BRICK 0xB53A15
#define COLOUR_MORTAR 0xBCAFA5
#define COLOUR_WOOD_RINGS 0xBC9862
#define COLOUR_LEAVES 0x50D937

#define KEY_ESCAPE 0xFF1B

/* Only the low 12 bits of a keysym are looked at when reading the keyboard. */
#define KEY_CODE_MASK 0xFFF
#define KEY_LEFT 0xF51
#define KEY_UP 0xF52
#define KEY_RIGHT 0xF53
#define KEY_DOWN 0xF54
#define KEY_F1 0xFBE
#define KEY_F32 0xFDD

#define TURN_STEP .05
#define SPEED_SCALE 42

/* The cached surfaces and the texels share one array, see render_pixel(). */
static Packed cache_and_textures[PIXEL_COUNT + TEXTURE_TABLE_SIZE];
#define surface_cache cache_and_textures
#define textures (cache_and_textures + PIXEL_COUNT)

static char voxels[VOXEL_COUNT];
static int frame[PIXEL_COUNT];

static Packed camera = START_POSITION;
static Packed camera_velocity;
static float yaw;
static float pitch;
static int speed;

/* The outcome of following one ray through the world. */
typedef struct {
    Packed texel;           /* colour of the face that was hit */
    Packed surface;         /* axis, voxel and texture coordinate of that face */
    float distance;         /* how far away the face is */
    float leaf_distance;    /* how far away the first leaves are */
} RayHit;

static double rotate(double sine_part, double cosine_part, double angle)
{
    return sine_part * sin(angle) + cosine_part * cos(angle);
}

/* Scale a vector and convert it to three signed lanes of fixed point. */
static Packed pack_vector(const float vector[AXIS_COUNT], float scale)
{
    Packed packed = 0;
    int axis;

    for (axis = 0; axis < AXIS_COUNT; axis++) {
        packed += (long long)(scale * vector[axis]) << (axis * LANE_BITS);
    }
    return packed;
}

/* Keep the 6 bit voxel coordinate of each lane of a position. */
static int voxel_index(Packed position)
{
    return position >> 10 & 0x3F | position >> 24 & 0xFC0 | position >> 38 & 0x3F000;
}

/* The voxel one step in the negative direction, wrapping around the world. */
static int voxel_below(int voxel, int axis)
{
    int shift = WORLD_SIZE_BITS * axis;
    int mask = (WORLD_SIZE - 1) << shift;

    return voxel & ~mask | voxel - (1 << shift) & mask;
}

/* Take the fractions of the two lanes that lie along a face hit on an axis. */
static Packed face_coordinate(Packed position, int axis, int flip)
{
    if (axis != AXIS_Y) {
        /* The fraction along the axis is 0, so OR picks the other of x and z. */
        return (position | position >> 40) & FRACTION_MASK
               | position >> 8 & TEXTURE_V_MASK
               | TEXTURE_SIDE_FLAG;
    }
    return position & FRACTION_MASK
           | position >> 28 & TEXTURE_V_MASK
           | (Packed)flip << TEXTURE_FLIP_SHIFT;
}

/* Keep the top 4 bits of u and v and the face flags to choose a texel. */
static Packed texture_lookup(Packed coordinate, int block)
{
    return textures[(coordinate >> 6 & 0xF | coordinate >> 14 & 0x3F0)
                    + block * TEXELS_PER_BLOCK];
}

/*
 * Walk along the ray once for each axis, visiting only the places where it
 * crosses from one voxel to the next along that axis. The three walks share the
 * closest hit so far, which also ends the later walks early.
 */
static void trace_ray(const float direction[AXIS_COUNT], RayHit *hit)
{
    int axis;

    hit->texel = 0;
    hit->surface = 0;
    hit->distance = FAR_DISTANCE;
    hit->leaf_distance = FAR_DISTANCE;

    for (axis = 0; axis < AXIS_COUNT; axis++) {
        float component = direction[axis];
        float crossing_length = VOXEL_SIZE / fabs(component);
        int negative = component < 0;
        Packed lane = camera >> (LANE_BITS * axis);
        /* How much of a voxel lies between the camera and the first crossing. */
        float first_crossing = ((negative ? lane : -lane) & FRACTION_MASK) / 1024.;
        float crossing[AXIS_COUNT];
        Packed position;
        Packed stride;
        float distance;
        int i;

        for (i = 0; i < AXIS_COUNT; i++) {
            crossing[i] = direction[i] * crossing_length;
        }
        crossing[axis] = round(crossing[axis]);

        position = camera + pack_vector(crossing, first_crossing);
        stride = pack_vector(crossing, 1);

        for (distance = crossing_length * first_crossing;
             distance < hit->distance;
             position += stride, distance += crossing_length) {
            int voxel = voxel_index(position);
            int below = voxel_below(voxel, axis);
            int entered = negative ? below : voxel;
            int left = negative ? voxel : below;
            /* Leaves can be seen through, so look at the far side of them too. */
            int visible = voxels[left] ? left : entered;
            int block = voxels[visible];

            if (block != BLOCK_EMPTY) {
                Packed coordinate = face_coordinate(position, axis, negative ^ (visible == left));
                Packed texel = texture_lookup(coordinate, block);

                /* The holes in the leaves are black texels, which do not stop a ray. */
                if (texel) {
                    hit->texel = texel;
                    hit->surface = axis | visible * 4 | coordinate << 32;
                    hit->distance = distance;
                }
                if (block == BLOCK_LEAVES && distance < hit->leaf_distance) {
                    hit->leaf_distance = distance;
                }
            }
        }
    }
}

/* Dim all three channels with one multiplication and put them in the frame. */
static void draw_pixel(int pixel, Packed texel, Packed brightness)
{
    Packed colour = texel * brightness >> 8;

    frame[pixel] = colour >> 32 << 8 | colour & 0xFF00FF;
}

static void render_traced_pixel(int pixel, int row, int column, int is_center)
{
    float row_position = (row - WINDOW_HEIGHT / 2.) / WINDOW_HEIGHT;
    float column_position = (column - WINDOW_WIDTH / 2.) / WINDOW_HEIGHT;
    float forward = rotate(row_position, 1, pitch);
    float direction[AXIS_COUNT];
    RayHit hit;
    Packed brightness;
    int leaves_in_front;

    direction[0] = rotate(forward, column_position, yaw);
    direction[1] = rotate(-1, row_position, pitch);
    direction[2] = rotate(-column_position, forward, yaw);

    /* The camera moves wherever the middle of the screen is looking. */
    if (is_center) {
        camera_velocity = pack_vector(direction, speed * SPEED_SCALE);
    }

    trace_ray(direction, &hit);

    /* Each axis has its own shade, and everything fades into the distance. */
    brightness = (255 - (int)(((hit.surface & 3) + 2) % 3) * 51) * (1 - hit.distance / FAR_DISTANCE);
    draw_pixel(pixel, hit.texel, brightness);

    leaves_in_front = hit.leaf_distance <= hit.distance;
    surface_cache[pixel] = 3 * leaves_in_front | hit.surface | brightness << 56;
}

/* Blend the texture coordinates of two pixels on the same face of a voxel. */
static void render_blended_pixel(int pixel, Packed before, Packed after, int weight)
{
    Packed before_coordinate = before << 8 >> 40;
    Packed after_coordinate = after << 8 >> 40;
    Packed coordinate = (weight * after_coordinate + (4 - weight) * before_coordinate >> 2)
                        & TEXTURE_CARRY_MASK;
    unsigned axis_and_voxel = before;
    Packed brightness = before >> 56;

    draw_pixel(pixel, texture_lookup(coordinate, voxels[axis_and_voxel / 4]), brightness);
    surface_cache[pixel] = axis_and_voxel | coordinate << 32 | brightness << 56;
}

/*
 * Only every fourth pixel in each direction is always traced. The others sit
 * between two finished pixels, and can copy them when those show the same face.
 */
static void render_pixel(int row, int column)
{
    int pixel = column + row * WINDOW_WIDTH;
    int stride = 1;
    int weight = column % INTERLEAVE;
    int is_center = row == WINDOW_HEIGHT / 2 && column == WINDOW_WIDTH / 2;
    Packed before;
    Packed after;
    Packed difference;
    int leaves;

    if (weight == 0) {
        weight = row % INTERLEAVE;
        stride = WINDOW_WIDTH;
    }

    /*
     * These can be one row past the end of the cache for the last pixels of
     * the screen, and that reads the first texels in the table. The array is
     * shared so that this stays well defined and gives the same pictures.
     */
    before = surface_cache[pixel - stride * weight];
    after = surface_cache[pixel + (INTERLEAVE - weight) * stride];

    difference = before ^ after;
    leaves = (before & 3) == 3;

    if (weight == 0 || is_center || (unsigned)difference != 0
        || leaves && (difference >> 38 & 0x3F00F)) {
        render_traced_pixel(pixel, row, column, is_center);
    } else {
        render_blended_pixel(pixel, before, after, weight);
    }
}

static void render_frame(void)
{
    int row_step;
    int column_step;

    for (row_step = 0; row_step < WINDOW_HEIGHT + INTERLEAVE - 1; row_step++) {
        int row = row_step % ROW_STRIDE * INTERLEAVE | row_step / ROW_STRIDE;

        for (column_step = 0; column_step < WINDOW_WIDTH + INTERLEAVE - 1; column_step++) {
            int column = column_step % COLUMN_STRIDE * INTERLEAVE | column_step / COLUMN_STRIDE;

            if (row < WINDOW_HEIGHT && column < WINDOW_WIDTH) {
                render_pixel(row, column);
            }
        }
    }
}

/* Pick the texel's colour, and change the brightness for the wood and grass. */
static Packed block_colour(int block, Packed x, Packed y, Packed *brightness)
{
    /* The grass reaches a little way down the side of the block. */
    Packed grass_edge = x * x * 3 + x * 81 / 4 & 3;
    static const Packed low_block_colours[] = {0x250DB1, 0x7F7F7F, 0x4040FF, 0x675231};
    Packed colour;

    if (block < 4) {
        colour = low_block_colours[block];
    } else if (block == BLOCK_BRICK) {
        colour = (x + y / 4 * 4) % 8 && y % 4 ? COLOUR_BRICK : COLOUR_MORTAR;
    } else if (block == BLOCK_LEAVES) {
        colour = rand() % 2 * COLOUR_LEAVES;
    } else {
        colour = COLOUR_DIRT;
    }

    if (block == BLOCK_GRASS) {
        if (y < grass_edge + 18) {
            colour = COLOUR_GRASS;
        } else if (grass_edge > y - 19) {
            /* y is unsigned, so this is the row under the grass, but not y = 18. */
            *brightness *= 0.7;
        }
    }

    if (block == BLOCK_WOOD) {
        /* The rings on the ends of a log have a one texel border. */
        if (((x - 1) & 15) < 14 && ((y - 1) & 15) < 14 && !(y & 16)) {
            int ring_x = 7 - (int)x;
            int ring_y = 7 - (int)(y % 16);

            ring_x ^= ring_x >> 31;
            ring_y ^= ring_y >> 31;
            colour = COLOUR_WOOD_RINGS;
            *brightness = 196 - rand() % 32 + (ring_y > ring_x ? ring_y : ring_x) % 3 * 42;
        } else {
            /* The bark is made of alternating light and dark stripes. */
            *brightness *= 1 + rand() % 2 * (.5 - (x & 1));
        }
    }
    return colour;
}

/* Make one texel. The brightness carries on from one texel to the next. */
static Packed make_texel(int block, Packed x, Packed y, Packed *brightness)
{
    Packed colour;
    int shade_roll = rand() % 3;

    /* Stone only changes shade on a third of the texels, which gives streaks. */
    if (block != BLOCK_STONE || shade_roll == 0) {
        *brightness = rand() % 96 ^ 255;
    }
    colour = block_colour(block, x, y, brightness);

    /* Spread the channels apart, dim them, and the bottom face is darker still. */
    return (colour & 0xFF00FF | (colour & 0xFF00) << 24) * (*brightness >> (y >> 5)) >> 8
           & 0xFF00FF00FF;
}

/*
 * Blocks become more likely further from the line through the middle of the
 * world, and this leaves a long tunnel along the x axis.
 */
static void generate_world(void)
{
    Packed brightness = 250;
    int index;

    for (index = VOXEL_COUNT - 1; index >= 0; index--) {
        int block_type = rand() % BLOCK_TYPES;
        int roll = rand() % 4;
        int distance_y = abs((index >> 6 & 63) - WORLD_CENTER);
        int distance_z = abs((index >> 12 & 63) - WORLD_CENTER);
        int block = index / TEXELS_PER_BLOCK;

        voxels[index] = roll < distance_y + distance_z - 3 ? block_type : BLOCK_EMPTY;

        /* The texture table is filled in during the last part of the loop. */
        if (block < BLOCK_TYPES) {
            textures[index] = make_texel(block, index % TEXTURE_SIZE,
                                         index % TEXELS_PER_BLOCK / TEXTURE_SIZE, &brightness);
        }
    }
}

static void handle_key(KeySym key)
{
    int code = key & KEY_CODE_MASK;

    switch (code) {
    case KEY_LEFT:
        yaw -= TURN_STEP;
        break;
    case KEY_RIGHT:
        yaw += TURN_STEP;
        break;
    case KEY_UP:
        pitch += TURN_STEP;
        break;
    case KEY_DOWN:
        pitch -= TURN_STEP;
        break;
    }

    /* F1 stops, and each function key after it moves faster. */
    if (code >= KEY_F1 && code <= KEY_F32) {
        speed = code - KEY_F1;
    }
}

int main(void)
{
    Display *display = XOpenDisplay(NULL);
    Window window = XCreateSimpleWindow(display, RootWindow(display, 0), 0, 0,
                                        WINDOW_WIDTH, WINDOW_HEIGHT, 1, 0, 0);
    XImage *image = XCreateImage(display, DefaultVisual(display, 0), 24, ZPixmap, 0,
                                 (char *)frame, WINDOW_WIDTH, WINDOW_HEIGHT, 32, 0);
    XEvent event;
    KeySym key = 0;

    XMapWindow(display, window);
    generate_world();

    while (XSelectInput(display, window, KeyPressMask) && key != KEY_ESCAPE) {
        if (XCheckWindowEvent(display, window, KeyPressMask, &event)) {
            key = XLookupKeysym(&event.xkey, 0);
            handle_key(key);
        } else {
            render_frame();
            camera += camera_velocity;

            /* The player digs through anything they move into. */
            voxels[voxel_index(camera)] = BLOCK_EMPTY;

            XPutImage(display, window, DefaultGC(display, 0), image, 0, 0, 0, 0,
                      WINDOW_WIDTH, WINDOW_HEIGHT);
        }
    }
    return 0;
}
