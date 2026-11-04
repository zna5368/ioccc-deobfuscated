#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

/*
 * Wolfenstein style ray casting engine.
 *
 * The world is a grid of 256x256 cells.  Every wall letter in the map becomes
 * a square block made of four wall segments.  For every screen column a ray
 * is cast against all wall segments and the nearest hit decides the height
 * and texture column of the wall slice drawn in that column.
 */

#define CELL_SIZE 256            /* size of one map cell in world units */
#define TEXTURE_SIZE 64          /* textures are 64x64 pixels per frame */
#define BMP_HEADER_PIXELS 18     /* 54 byte BMP header = 18 RGB pixels */
#define BYTES_PER_FRAME 12288    /* 64 * 64 * 3 bytes of pixels per frame */
#define FRAME_USEC 49152         /* time between rendered frames */
#define NO_HIT_DISTANCE (12288 << 8)
#define MIN_STEP_DISTANCE 200    /* keep this far away from walls */
#define CEILING_COLOR 0x4453D0
#define FLOOR_COLOR (CEILING_COLOR << 1)

/*
 * The sine table holds one row of 256 angles for every screen column.  128
 * steps make (roughly) a full turn, so adding QUARTER_TURN gives the cosine
 * and adding HALF_TURN looks backwards.  Each row is shifted by the viewing
 * angle of its screen column, so row "column" at angle "a" is the direction
 * of the ray through that column.
 */
#define ANGLE_ROW 256
#define QUARTER_TURN 32
#define HALF_TURN 64
#define ANGLE_MASK 127

/* keyboard state indexed by the low 6 bits of the key symbol */
#define KEY_LEFT (XK_Left & 63)
#define KEY_UP (XK_Up & 63)
#define KEY_RIGHT (XK_Right & 63)
#define KEY_DOWN (XK_Down & 63)

Window window;
GC gc;
Display *display;
XEvent event;
XImage *image;

float sine[7 << 16];
struct timeval now, sleep_time;

int frame_counter;
int angle = 92;
int player_x, player_y;
int hit_texture, hit_offset;
int width, height;

/* texture storage; initially filled with a default checkered texture */
int texture_pixels[7 << 16];
int *next_pixel = texture_pixels;
int *textures[96];      /* first pixel of each texture, indexed by letter - 10 */
int frames[96];         /* number of animation frames of each texture */
int *framebuffer;
int key_down[64];
int frame_deadline;

struct wall {
  int texture;
  int x, y;   /* start point */
  int dx, dy; /* unit direction; every segment is CELL_SIZE long */
} walls[9999];

/* map text; initially filled with a default maze */
unsigned char map_text[9999], *cursor = map_text;
unsigned char texture_name[] = " .bmp";
FILE *file;

/* texture loader state that carries over from one texture to the next */
int pixel;
unsigned char byte;

/* Cast a ray from the player along sine table entry "direction". Returns
 * the distance to the nearest wall and sets hit_texture and hit_offset (the
 * position along the hit wall, 0..255). */
int cast_ray(int direction) {
  float ray_x = sine[direction], ray_y = sine[direction + QUARTER_TURN];
  int nearest = NO_HIT_DISTANCE, along, distance;
  struct wall *wall;

  for (wall = walls; wall->x; wall++) {
    along = (int)((ray_y * (wall->x - player_x) - ray_x * (wall->y - player_y)) /
                  (ray_x * wall->dy - ray_y * wall->dx));
    if (along >> 8)
      continue; /* the ray passes beside this wall segment */
    distance = (int)((wall->dy * (wall->x - player_x) -
                      wall->dx * (wall->y - player_y)) /
                     (ray_x * wall->dy - ray_y * wall->dx));
    if (distance > 0 && distance < nearest) {
      nearest = distance;
      hit_offset = along;
      hit_texture = wall->texture;
    }
  }
  return nearest;
}

struct wall *add_wall(struct wall *wall, int texture, int x, int y, int dx,
                      int dy) {
  wall->x = x;
  wall->y = y;
  wall->dx = dx;
  wall->dy = dy;
  wall->texture = texture;
  return wall + 1;
}

/* Read a 24 bit BMP file into next_pixel. Frames of an animated texture are
 * placed side by side in the bitmap. */
void load_texture(int id) {
  int count = 2;

  frames[id] = 1;
  textures[id] = next_pixel;
  file = fopen(texture_name, "r");
  if (!file)
    return; /* use whatever is in texture_pixels (the default texture) */

  textures[id] = ++next_pixel + 1 + BMP_HEADER_PIXELS;
  do {
    /* collect 3 bytes (blue, green, red) into one 0xRRGGBB pixel */
    if (count++ % 3 == 0) {
      pixel = 0;
      ++next_pixel;
    }
    pixel = pixel >> 8 | byte << 16;
    *next_pixel = pixel;
  } while (fread(&byte, 1, 1, file) > 0);
  fclose(file);

  count /= BYTES_PER_FRAME;
  frames[id] = count ? count : 1;
}

int main(int argc, char **argv) {
  int i, column, row, step, direction, wall_height, texel_row;
  int cell_x = 0, cell_y = 0, delay = 0, tex_stride = 0;
  int *texture_column = 0;
  struct wall *wall, *other, *kept, *walls_end;

  /* usage: vik [-w width] [mapfile] */
  argc--;
  width = (argc < 2 || argv[1][1] != 'w') ? 320 : atoi(argv[2]);
  height = 1 | 3 * width / 4;

  player_x = player_y = 6144;
  display = XOpenDisplay(0);
  /* fill the sine table, the default maze (rows of 'f' blocks) and the
   * default checkered texture used when a .bmp file cannot be opened */
  for (i = 0; i < width << 8; i++) {
    sine[i] = sin((i & 255) / 20.4 + asin((i / ANGLE_ROW - width / 2) / 3.2 / width));
    map_text[i & 511] = (i & 9) == 9 ? '\n' : 'f';
    texture_pixels[i] = i / 18 << (16 & (i >> 6 ^ i));
  }

  window = XCreateSimpleWindow(display, RootWindow(display, 0), 0, 0, width,
                               height, 0, 0, 0);
  gc = XCreateGC(display, window, 0, 0);

  /* the map file is the last argument; it overwrites the default maze */
  file = fopen(argc ? argv[argc] : "", "r");
  if (file) {
    fread(cursor + 1, 1, 1 << 15, file);
    fclose(file);
  }

  /* build wall segments behind the space reserved for the final list */
  walls_end = walls + 4096;
  for (cursor = map_text + 1; *cursor; cursor++) {
    int id = *cursor - 10;

    texture_name[0] = *cursor;
    if (*cursor == '\n') {
      cell_x = 0;
      cell_y += CELL_SIZE;
    }
    cell_x += CELL_SIZE;

    if (*cursor >= 'A' && *cursor <= 'i') {
      /* four sides of a block, going clockwise */
      walls_end = add_wall(walls_end, id, cell_x, cell_y, 1, 0);
      walls_end = add_wall(walls_end, id, cell_x + CELL_SIZE, cell_y, 0, 1);
      walls_end = add_wall(walls_end, id, cell_x + CELL_SIZE,
                           cell_y + CELL_SIZE, -1, 0);
      walls_end = add_wall(walls_end, id, cell_x, cell_y + CELL_SIZE, 0, -1);
      if (!textures[id])
        load_texture(id);
    }
    if (*cursor == '*') {
      player_x = cell_x + 99;
      player_y = cell_y + 99;
    }
  }

  XMapWindow(display, window);
  XSelectInput(display, window, KeyPressMask | KeyReleaseMask);

  /* copy the walls to the front, dropping pairs of walls that lie between
   * two neighbouring blocks (same segment, opposite direction) */
  kept = walls;
  for (wall = walls + 4096; wall->x; wall++) {
    for (other = walls + 4096; other->x; other++)
      if (other != wall &&
          other->x == wall->x - 128 * (other->dx - wall->dx) &&
          other->y == wall->y - 128 * (other->dy - wall->dy))
        break;
    *kept = *wall;
    if (!other->x)
      kept++;
  }

  framebuffer = malloc(4 * width * height);
  image = XCreateImage(display, DefaultVisual(display, 0),
                       DefaultDepth(display, 0), ZPixmap, 0,
                       (char *)framebuffer, width, height, 32, 0);

  for (;;) {
    /* turn and move with the cursor keys */
    angle = (angle + key_down[KEY_RIGHT] - key_down[KEY_LEFT]) & ANGLE_MASK;
    step = key_down[KEY_UP] - key_down[KEY_DOWN];
    if (step) {
      direction = width * 128 + angle + (step < 0) * HALF_TURN;
      if (cast_ray(direction) > MIN_STEP_DISTANCE) {
        player_x += (int)(sine[direction] * 64);
        player_y += (int)(sine[direction + QUARTER_TURN] * 64);
      }
    }

    /* wait for the next frame */
    if (delay > 0 && delay < 4 * BYTES_PER_FRAME) {
      sleep_time.tv_usec = delay;
      select(0, 0, 0, 0, &sleep_time);
      frame_deadline += FRAME_USEC;
    } else {
      frame_deadline = now.tv_usec + FRAME_USEC;
    }
    frame_counter = frame_counter + 1 & -1 >> 1;

    for (column = 0; column < width; column++) {
      hit_texture = 0;
      wall_height = 1 + (int)(height * 512 /
                              (cast_ray(column * ANGLE_ROW + angle) *
                               sine[column * ANGLE_ROW + QUARTER_TURN]));
      if (hit_texture) {
        tex_stride = TEXTURE_SIZE * frames[hit_texture];
        texture_column = textures[hit_texture] + hit_offset / 4 +
                         TEXTURE_SIZE * (frame_counter / 3 % frames[hit_texture]);
      }
      for (row = 0; row < height; row++) {
        texel_row = 32 - (17 << (wall_height < 2)) * (2 * row - height) / wall_height;
        if (texel_row >> 6)
          framebuffer[column + width * row] =
              texel_row < 0 ? FLOOR_COLOR : CEILING_COLOR;
        else
          framebuffer[column + width * row] = texture_column[tex_stride * texel_row];
      }
    }

    XPutImage(display, window, gc, image, 0, 0, 0, 0, width, height);
    while (XPending(display)) {
      XNextEvent(display, &event);
      key_down[63 & XLookupKeysym(&event.xkey, 0)] ^=
          event.type == KeyPress || event.type == KeyRelease;
    }
    XSync(display, 0);
    gettimeofday(&now, 0);
    delay = frame_deadline - now.tv_usec;
  }
}
