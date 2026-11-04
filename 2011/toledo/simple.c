#include <X11/Xlib.h>
#include <X11/keysym.h>

/* size of one player's view; the window holds two views side by side */
#define VIEW_WIDTH 512
#define VIEW_HEIGHT 288

/*
 * The Makefile defines the keys as "player2_key:player1_key" pairs:
 *   A = forward, B = turn left, C = turn right, D = backward, E = fire
 * so (player ? A) picks the key belonging to that player.
 */
#define KEYS_FORWARD A
#define KEYS_LEFT B
#define KEYS_RIGHT C
#define KEYS_BACKWARD D
#define KEYS_FIRE E
#define PLAYER_KEY(player, keys) ((player) ? keys)

/* headings go from 0 to 71 (5 degrees per step); 11.46 = 72 / (2 * pi) */
#define FULL_TURN 72
float angle_unit = 11.46;

/* object kinds stored in object.flags */
#define TANK 1
#define OBSTACLE 2
#define BULLET 4

/* object slots: 0 = red player, 1 = green player, 2 = cyan target tank,
   3..10 = obstacles, 11..20 = bullets */
#define NUM_OBJECTS 21
#define FIRST_BULLET 11
#define MAX_SHOTS 3

/* bullet timer values */
#define BULLET_LIFETIME 64
#define BULLET_GONE 71

/* number of ints in a tank model, used for the per-object explosion copy */
#define MODEL_SIZE 193

/* color of the cyan tank and of the obstacles */
#define CYAN 0xFDDB
#define YELLOW 0xFDDB00
#define ORANGE 0xFFC000
#define BLUE 0xFF
#define DARK_BLUE 0x80
#define BLACK 0

/*
 * Every model is a count of line segments followed by the two end points
 * (x, y, z) of each segment.  y grows downwards and 0 is the ground.
 */
// clang-format off
/* cube obstacle */
int cube_model[] = {
    12, /* number of line segments */
    /* x, y, z of start point      x, y, z of end point */
     -20,    0,  -20,     20,    0,  -20,
      20,    0,  -20,     20,    0,   20,
      20,    0,   20,    -20,    0,   20,
     -20,    0,   20,    -20,    0,  -20,
     -20,  -40,  -20,     20,  -40,  -20,
      20,  -40,  -20,     20,  -40,   20,
      20,  -40,   20,    -20,  -40,   20,
     -20,  -40,   20,    -20,  -40,  -20,
     -20,    0,  -20,    -20,  -40,  -20,
      20,    0,  -20,     20,  -40,  -20,
      20,    0,   20,     20,  -40,   20,
     -20,    0,   20,    -20,  -40,   20,
};

/* pyramid obstacle */
int pyramid_model[] = {
    8, /* number of line segments */
    /* x, y, z of start point      x, y, z of end point */
     -20,    0,  -20,     20,    0,  -20,
      20,    0,  -20,     20,    0,   20,
      20,    0,   20,    -20,    0,   20,
     -20,    0,   20,    -20,    0,  -20,
     -20,    0,  -20,      0,  -40,    0,
      20,    0,  -20,      0,  -40,    0,
      20,    0,   20,      0,  -40,    0,
     -20,    0,   20,      0,  -40,    0,
};

/* bullet: a small square */
int bullet_model[] = {
    4, /* number of line segments */
    /* x, y, z of start point      x, y, z of end point */
      -2,  -12,    0,      2,  -12,    0,
       2,  -12,    0,      2,  -13,    0,
       2,  -13,    0,     -2,  -13,    0,
      -2,  -13,    0,     -2,  -12,    0,
};

/* tank: hull, turret and gun barrel */
int tank_model[] = {
    32, /* number of line segments */
    /* x, y, z of start point      x, y, z of end point */
     -20,    0,  -27,     20,    0,  -27,
      20,    0,  -27,     20,    0,   27,
      20,    0,   27,    -20,    0,   27,
     -20,    0,   27,    -20,    0,  -27,
     -20,  -10,  -30,     20,  -10,  -30,
      20,  -10,  -30,     20,  -10,   30,
      20,  -10,   30,    -20,  -10,   30,
     -20,  -10,   30,    -20,  -10,  -30,
     -20,    0,  -27,    -20,  -10,  -30,
      20,    0,  -27,     20,  -10,  -30,
      20,  -10,   30,     20,    0,   27,
     -20,  -10,   30,    -20,    0,   27,
     -10,  -10,  -15,     10,  -10,  -15,
      10,  -10,  -15,     10,  -10,   15,
      10,  -10,   15,    -10,  -10,   15,
     -10,  -10,   15,    -10,  -10,  -15,
      -8,  -15,  -13,      8,  -15,  -13,
       8,  -15,  -13,      8,  -15,   13,
       8,  -15,   13,     -8,  -15,   13,
      -8,  -15,   13,     -8,  -15,  -13,
     -10,  -10,  -15,     -8,  -15,  -13,
      10,  -10,  -15,      8,  -15,  -13,
      10,  -10,   15,      8,  -15,   13,
     -10,  -10,   15,     -8,  -15,   13,
      -1,  -12,  -14,     -1,  -12,  -32,
       1,  -12,  -14,      1,  -12,  -32,
       1,  -13,  -14,      1,  -13,  -32,
      -1,  -13,  -14,     -1,  -13,  -32,
      -1,  -12,  -32,      1,  -12,  -32,
       1,  -12,  -32,      1,  -13,  -32,
       1,  -13,  -32,     -1,  -13,  -32,
      -1,  -13,  -32,     -1,  -12,  -32,
};

/* mountain outline on the horizon: (step to the right, height) pairs;
   a step of 0 starts a new line without drawing */
int mountains[] = {
      0,  10,
      4,   3,
      6,   8,
      4,  11,
      2,   6,
      4,   3,
     12,   9,
      8,   5,
      8,  10,
      6,   4,
      4,   3,
      4,   8,
      2,  10,
      4,   3,
      6,   8,
      4,  11,
      2,   6,
      4,   3,
     12,   9,
};
// clang-format on

struct object {
  int hits; /* times a tank was hit; 3 destroys it */
  int color;
  int timer; /* tank: recoil/reload countdown or explosion frame,
                bullet: age */
  int *model;
  int x;       /* position in 1/256 units */
  int heading; /* 0..71 */
  int flags;   /* TANK, OBSTACLE, BULLET or 0 for an empty slot */
  int shots;   /* tank: bullets still flying */
  int owner;   /* bullet: the tank that fired it */
  int z;       /* position in 1/256 units */
} objects[NUM_OBJECTS];

/* scattered copy of a model while an object explodes */
int debris[NUM_OBJECTS][MODEL_SIZE];

/* score of each tank; it makes the right part of the status bar grow */
int scores[3];

/* state of every key, indexed by its keysym */
int key_down[1 << 28];

int playing;      /* 0 while the spinning tank splash screen is shown */
int splash_angle; /* counts 0..71 while the splash screen is shown */

Display *display;
GC gc;
Pixmap pixmap;
Window window;

/* result of rotate() */
int rot_x, rot_z;

/* rotate the point (rot_x, z) around the vertical axis */
void rotate(int angle, int z) {
  int x = rot_x;
  rot_x = x * cos(angle / angle_unit) - z * sin(angle / angle_unit);
  rot_z = z * cos(angle / angle_unit) + x * sin(angle / angle_unit);
}

/*
 * Draw a model turned by heading, placed at (dx, dz) from the viewer
 * and seen by a viewer looking towards view_angle.
 */
void draw_model(int view_angle, int heading, int dx, int dz, int *model) {
  int *data = model;
  int points = 2 * *data++;
  int start_x = 0, start_y = 0, start_depth = 0;

  do {
    int x = *data++;
    int y = 12 + *data++; /* the eye is 12 units above the ground */
    int z = *data++;
    int depth, divisor, screen_x, screen_y;

    rot_x = x;
    rotate(heading, z);
    rot_x += dx;
    rotate(view_angle, rot_z + dz);

    depth = rot_z;
    divisor = 1 > depth ? 1 : depth;
    screen_x = rot_x * VIEW_HEIGHT / divisor + VIEW_WIDTH / 2;
    screen_y = VIEW_HEIGHT / 2 + y * VIEW_HEIGHT / divisor;
    if (screen_x < -2 * VIEW_WIDTH)
      screen_x = -2 * VIEW_WIDTH;
    if (screen_x > 2 * VIEW_WIDTH)
      screen_x = 2 * VIEW_WIDTH;

    if (points & 1) {
      /* end point: draw unless the whole segment is behind the viewer */
      if (start_depth > 0 || depth > 0)
        XDrawLine(display, pixmap, gc, start_x, start_y, screen_x, screen_y);
    } else {
      start_depth = depth;
      start_x = screen_x;
      start_y = screen_y;
    }
  } while (--points);
}

void set_color(int color) {
  XGCValues values;
  values.foreground = color;
  values.line_width = 2;
  XChangeGC(display, gc, GCForeground | GCLineWidth, &values);
}

/* move every line segment of an exploding object a random amount */
void explode(int obj, int frame) {
  int *src = objects[obj].model;
  int *dst = objects[obj].model = debris[obj];
  int points = 2 * (*dst++ = *src++);
  int dx = 0, dy = 0, y;

  do {
    if (--points & 1) {
      /* new random offset for each segment; after frame 7 it falls */
      dx = rand() % 9 - 4;
      dy = rand() % 9 - 4 * (frame < 8);
    }
    *dst++ = *src++ + dx;
    y = *src++ + dy;
    *dst++ = y > 0 ? 0 : y; /* do not sink below the ground */
    *dst++ = *src++ + dx;
  } while (points);
}

/* move an object along its heading */
void move_forward(int obj, int distance) {
  rot_x = 0;
  rotate(objects[obj].heading, distance * 512);
  objects[obj].x -= rot_x;
  objects[obj].z -= rot_z;
}

/* return 1 + the slot of a tank or obstacle touching obj, or 0 */
int find_collision(int obj, int size) {
  int limit = 5120 * size;
  int other;

  for (other = FIRST_BULLET; other >= 0; other--)
    if (other != obj && abs(objects[other].x - objects[obj].x) <= limit &&
        objects[other].flags >= 1 && objects[other].hits <= 2 &&
        abs(objects[other].z - objects[obj].z) <= limit)
      return other + 1;
  return 0;
}

void new_level(void) {
  int c;

  for (c = FIRST_BULLET - 1; c >= 0; c--) {
    objects[c].hits = objects[c].timer = 0;
    objects[c].flags = c > 2 ? OBSTACLE : TANK;
    do {
      objects[c].x = (rand() % 978 - 499) * 256;
      objects[c].heading = rand() % 71;
      objects[c].z = (rand() % 979 - 501) * 256;
    } while (find_collision(c, 2));
    if (c > 2)
      objects[c].model = rand() % 2 ? pyramid_model : cube_model;
    else
      objects[c].model = tank_model;
    if (c < 2)
      objects[c].color = 0xF9 << (16 - c * 8); /* red or green */
    else
      objects[c].color = c > 2 ? YELLOW : CYAN;
  }
}

void fire(int c) {
  int b = FIRST_BULLET - 1;

  while (objects[++b].flags)
    ;
  move_forward(c, -4); /* recoil */
  objects[b] = objects[c];
  objects[b].owner = c;
  objects[c].shots++;
  objects[c].timer = 7;
  objects[b].model = bullet_model;
  objects[b].flags = BULLET;
  move_forward(b, 19); /* start in front of the gun */
  objects[b].color = ORANGE;
}

/* returns the number of things still moving */
int update_world(void) {
  int busy = 0;
  int c, hit, speed;

  for (c = FIRST_BULLET; c < NUM_OBJECTS - 1; c++) {
    if (!(objects[c].flags & BULLET))
      continue;
    ++objects[c].timer;
    if (objects[c].timer == BULLET_LIFETIME ||
        objects[c].timer == BULLET_GONE) {
      objects[c].flags = 0;
      objects[objects[c].owner].shots--;
    } else {
      busy = 1;
      if (objects[c].timer < BULLET_LIFETIME) {
        if (hit = find_collision(c, 1)) {
          objects[c].timer = BULLET_LIFETIME;
          if (hit < 4) {
            scores[objects[c].owner]++;
            ++objects[hit - 1].hits;
          }
        } else
          move_forward(c, 7);
      } else
        explode(c, objects[c].timer - BULLET_LIFETIME - 1);
    }
  }

  for (c = 2; c >= 0; c--) {
    if (!(objects[c].flags & TANK))
      continue;
    busy++;
    if (objects[c].hits > 2) {
      if (objects[c].timer > 15)
        objects[c].flags = 0;
      else
        explode(c, objects[c].timer++);
    } else {
      if (objects[c].timer && objects[c].timer-- > 6)
        move_forward(c, 4); /* come back from the recoil */
      if (c < 2) {
        objects[c].heading = (FULL_TURN + objects[c].heading +
                              key_down[PLAYER_KEY(c, KEYS_LEFT)] -
                              key_down[PLAYER_KEY(c, KEYS_RIGHT)]) %
                             FULL_TURN;
        speed = 4 * key_down[PLAYER_KEY(c, KEYS_FORWARD)] -
                key_down[PLAYER_KEY(c, KEYS_BACKWARD)] * 4;
        if (speed) {
          move_forward(c, speed);
          if (find_collision(c, 2))
            move_forward(c, -speed);
        }
        if (key_down[PLAYER_KEY(c, KEYS_FIRE)] && !objects[c].timer &&
            objects[c].shots < MAX_SHOTS)
          fire(c);
      }
    }
  }
  return busy;
}

void draw_view(int viewer) {
  int view_angle = (108 - objects[viewer].heading) % FULL_TURN;
  int c, *step, x_units, x, y, last_x = 0, last_y = 0, lost;

  /* the background turns blue when this player's tank is destroyed */
  set_color(objects[viewer].hits == 3 ? DARK_BLUE : BLACK);
  XFillRectangle(display, pixmap, gc, 0, 0, VIEW_WIDTH, VIEW_HEIGHT);

  if (!playing) {
    set_color(0xFFFF);
    draw_model(FULL_TURN / 2, 54 - splash_angle, 0, -120, tank_model);
  } else {
    set_color(BLUE);
    XDrawLine(display, pixmap, gc, 0, VIEW_HEIGHT / 2, VIEW_WIDTH,
              VIEW_HEIGHT / 2);

    /* mountains scroll sideways as the player turns */
    x_units = -.889 * view_angle;
    for (step = mountains; step < mountains + 38; step += 2) {
      x_units += step[0];
      x = x_units * VIEW_WIDTH / 32;
      y = step[1] * VIEW_HEIGHT / 48;
      if (step[0])
        XDrawLine(display, pixmap, gc, last_x, last_y, x, y);
      last_x = x;
      last_y = y;
    }

    for (c = 0; c < NUM_OBJECTS; c++)
      if (objects[c].flags && c != viewer) {
        set_color(objects[c].color);
        draw_model(view_angle, objects[c].heading,
                   (objects[c].x - objects[viewer].x) >> 8,
                   (objects[c].z - objects[viewer].z) >> 8, objects[c].model);
      }

    /* status bars: energy on the left, score growing to the right;
       the bar of the viewing player is wider */
    for (c = 0; c < 3; c++) {
      y = 16 + 32 * c;
      set_color(objects[c].color);
      XFillRectangle(display, pixmap, gc, c == viewer ? 8 : 16, y,
                     (c == viewer ? 76 : 68) + scores[c], 16);
      set_color(BLACK);
      lost = 20 * objects[c].hits;
      XFillRectangle(display, pixmap, gc, 80 - lost, 4 + y, lost, 8);
    }
  }

  set_color(BLUE);
  XDrawLine(display, pixmap, gc, VIEW_WIDTH, 0, VIEW_WIDTH, VIEW_HEIGHT);
  XCopyArea(display, pixmap, window, gc, 0, 0, VIEW_WIDTH, VIEW_HEIGHT,
            viewer * VIEW_WIDTH, 0);
}

void game_frame(void) {
  int viewer;

  if (!playing) {
    splash_angle = (splash_angle + 1) % FULL_TURN;
    if (!splash_angle) {
      playing = 1;
      new_level();
    }
  }
  if (playing && update_world() < 2)
    playing = 0; /* only one tank left: go to the next level */

  for (viewer = 1; viewer >= 0; viewer--)
    draw_view(viewer);
}

int main() {
  XEvent event;

  display = XOpenDisplay(NULL);
  window = XCreateSimpleWindow(display, DefaultRootWindow(display), 64, 64,
                               VIEW_WIDTH * 2, VIEW_HEIGHT, 2, 0, 0);
  XSelectInput(display, window, KeyPressMask | KeyReleaseMask);
  XMapWindow(display, window);
  srand(time(0));
  pixmap = XCreatePixmap(display, window, VIEW_WIDTH, VIEW_HEIGHT,
                         DefaultDepth(display, 0));
  gc = XCreateGC(display, pixmap, 0, 0);

  for (;;) {
    game_frame();
    XFlush(display);
    usleep(33367); /* about 30 frames per second */
    while (XPending(display)) {
      XNextEvent(display, &event);
      /* every key press or release flips the key's state */
      if (event.type == KeyPress || event.type == KeyRelease)
        key_down[XLookupKeysym(&event.xkey, 0)] ^= 1;
    }
  }
}
