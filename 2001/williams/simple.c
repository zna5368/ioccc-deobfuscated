#include <X11/Xlib.h>
#include <unistd.h>

/*
 * Missile Command.
 *
 * All positions and speeds are fixed point numbers with FIX fraction bits,
 * so (x >> FIX) is the pixel coordinate.
 *
 * Everything is drawn with an XOR graphics context: drawing the same thing a
 * second time erases it.
 */

#define WIDTH 400
#define HEIGHT 300
#define MAX_OBJECTS 222
#define MAX_CITIES 7
#define FIX 9

/* object types */
#define DEAD 0
#define MISSILE 1          /* enemy missile, may split into a MIRV */
#define SMART_BOMB 2       /* enemy bomb that steers around explosions */
#define ERASE_TRAIL 3      /* finished MIRV parent whose trail is erased */
#define ABM 4              /* player's anti-ballistic missile */
#define EXPLOSION_SHRINK 8 /* explosion being erased (full size) */
#define EXPLOSION_GROW 16  /* explosion growing */

typedef struct {
  union {
    long dy;         /* y step (lines) */
    long max_radius; /* explosions */
  };
  long speed;      /* steps per frame */
  long steps_left; /* steps until the line ends / explosion size */
  long x, y;       /* current position */
  union {
    long dx;     /* x step (lines) */
    long radius; /* current radius (explosions) */
  };
  long type;
  union {
    long parent;   /* index+1 of the MIRV parent, 0 = none */
    long target_x; /* smart bombs */
  };
  long children; /* number of MIRV children still alive */
} Object;

Display *display;
Window window;
GC gc;
XEvent event;
char score_format[] = "Level %d   Score %d", message[222];

long delay, push_x, push_y, num_cities = 6, missiles_left, score = 0,
                                level = 0, next_slot = 0;
long city_x[MAX_CITIES] = {33, 99, 165, 231, 297, 363};

XGCValues gc_values = {
    .function = GXxor,
    .foreground = ~0L,
    .background = 0,
    .line_width = 1,
};

/* outlines, the first point is set before drawing, the rest are relative */
XPoint city_shape[] = {{0, HEIGHT}, {-20, 0}, {4, -20}, {4, 10},
                       {4, -5},     {4, 5},   {4, -20}, {4, 20},
                       {4, -5},     {4, 5},   {4, -10}, {4, 20}};
XPoint bomb_shape[] = {{0, 0}, {4, 0}, {-4, 4}, {-4, -4}, {4, -4}, {4, 4}};

/* current state of every object and a copy of how it started */
Object objects[MAX_OBJECTS], initial[MAX_OBJECTS];

void draw_city(long city) {
  city_shape[0].x = city_x[city];
  city_shape[0].y = HEIGHT;
  XFillPolygon(display, window, gc, city_shape, 12, Nonconvex,
               CoordModePrevious);
}

void draw_bomb(long x, long y) {
  bomb_shape[0].x = x;
  bomb_shape[0].y = y;
  if (y != 0)
    XFillPolygon(display, window, gc, bomb_shape, 6, Nonconvex,
                 CoordModePrevious);
}

/* store an object in the ring buffer, overwriting the oldest */
void add_object(Object obj) {
  initial[next_slot] = objects[next_slot] = obj;
  next_slot = (next_slot + 1) % MAX_OBJECTS;
}

void spawn_explosion(Object obj, long size) {
  obj.speed = 1;
  obj.radius = 1;
  obj.max_radius = size;
  obj.steps_left = size;
  obj.type = EXPLOSION_GROW;
  obj.parent = 0;
  add_object(obj);
}

/* launch a line from (x0,y0) to (x1,y1) moving `speed` pixels per frame */
void launch(long type, long x0, long y0, long x1, long y1, long speed,
            long parent) {
  Object obj;
  long dx, dy, abs_dx, abs_dy, length;

  obj.children = 0;
  obj.speed = speed;
  obj.type = type;
  obj.y = y0;
  obj.x = x0;
  if (type == SMART_BOMB)
    obj.target_x = x1;
  else
    obj.parent = parent;

  dx = x1 - x0;
  dy = y1 - y0;
  abs_dx = dx < 0 ? -dx : dx;
  abs_dy = dy < 0 ? -dy : dy;
  length = abs_dx > abs_dy ? abs_dx : abs_dy;

  obj.steps_left = length >> FIX;
  obj.dx = (dx << FIX) / length;
  obj.dy = (dy << FIX) / length;
  add_object(obj);
}

/*
 * Check whether a missile is inside an explosion. Returns the new score
 * (nonzero) on a hit, otherwise 0. Also works out push_x/push_y, which push
 * smart bombs away from nearby explosions.
 */
long check_hit(Object missile) {
  long dx, dy, radius, dist2, i = MAX_OBJECTS;
  Object ex;

  push_x = push_y = 0;
  while (i--) {
    ex = objects[i];
    if (ex.type >= EXPLOSION_SHRINK) {
      dx = (missile.x - ex.x) >> FIX;
      dy = (missile.y - ex.y) >> FIX;
      radius = ex.type == EXPLOSION_SHRINK ? ex.max_radius : ex.radius;
      dist2 = dx * dx + dy * dy;
      if (dist2 < radius * radius ||
          (missile.type == SMART_BOMB && dist2 < 26))
        return score += 10;
      dist2 = (20 << FIX) / (dist2 | 1);
      push_x += dx * dist2;
      push_y += dy * dist2;
    }
  }
  return 0;
}

/* index of a city within `dist` pixels of x, or -1 */
long find_city(long x, long dist) {
  long i = num_cities;
  while (i-- && (x < city_x[i] - dist || x > city_x[i] + dist))
    ;
  return i;
}

/* a MIRV child is gone; erase the parent's trail once all children are */
void child_finished(long parent) {
  if (parent) {
    parent--;
    if (--objects[parent].children == 0)
      objects[parent].type = ERASE_TRAIL;
  }
}

/* move and draw every object for one frame; returns the number alive */
long draw_objects(void) {
  long steps, alive = 0, i = MAX_OBJECTS, y, radius, city;
  Object obj;

  while (i--) {
    obj = objects[i];
    if (obj.type == DEAD)
      continue;
    alive++;
    steps = obj.speed;

    if (obj.type == ERASE_TRAIL) {
      /* redraw the whole trail at once to erase it */
      steps = obj.steps_left;
      obj.type = DEAD;
      child_finished(obj.parent);
    }

    while (steps--) {
      --obj.steps_left;
      y = obj.y >> FIX;

      if (obj.type >= EXPLOSION_SHRINK) {
        radius = obj.radius++;
        XDrawArc(display, window, gc, (obj.x >> FIX) - obj.radius, y - radius,
                 radius * 2, radius * 2, 0, 360 * 64);
        if (obj.steps_left == 0) {
          /* grow -> shrink (erase) -> dead */
          initial[i].type -= 8;
          obj = initial[i];
        }
      } else if (obj.type == SMART_BOMB) {
        draw_bomb(obj.x >> FIX, y); /* erase the old bomb */
      } else {
        obj.x += obj.dx;
        obj.y += obj.dy;
        y = obj.y >> FIX;
        XDrawPoint(display, window, gc, obj.x >> FIX, y);
      }

      if (obj.type == ABM && obj.steps_left == 0) {
        /* ABM reached its target: explode, then erase its trail */
        spawn_explosion(obj, 20);
        steps = initial[i].steps_left - obj.steps_left;
        obj = initial[i];
        obj.type = DEAD;
      }

      if ((obj.type == MISSILE || obj.type == SMART_BOMB) &&
          (check_hit(obj) || y > HEIGHT)) {
        if (y > HEIGHT && (city = find_city(obj.x >> FIX, 25)) >= 0) {
          draw_city(city); /* erase it */
          city_x[city] = city_x[--num_cities];
        }
        spawn_explosion(obj, 30);
        if (obj.type == MISSILE) {
          child_finished(obj.parent);
          steps = initial[i].steps_left - obj.steps_left;
          obj = initial[i];
          obj.type = DEAD;
        } else {
          steps = 0;
          obj.type = DEAD;
        }
      }

      if (obj.type == MISSILE && y < HEIGHT - 75 &&
          random() % (delay * 77) == 0) {
        /* split into a MIRV */
        do {
          launch(MISSILE, obj.x, obj.y, random() % (WIDTH << FIX),
                 HEIGHT << FIX, 1, i + 1);
          initial[i].children++;
        } while (random() % 3);
        /* wait (dead) until the children finish, then erase the trail */
        steps = initial[i].steps_left - obj.steps_left;
        obj = initial[i];
        obj.type = DEAD;
        obj.steps_left = steps;
        steps = 0;
      }

      if (obj.type == SMART_BOMB) {
        /* steer toward the target and away from explosions */
        obj.x += obj.dx + push_x;
        obj.dx = (obj.target_x - obj.x) / ((HEIGHT + 20 - y) | 1);
        obj.y += obj.dy + push_y;
        draw_bomb(obj.x >> FIX, obj.y >> FIX);
      }
    }
    objects[i] = obj;
  }
  return alive;
}

void play(void) {
  long i, alive, type, start_x, target_x, length;

  while (num_cities) {
    XFlush(display);
    if (level)
      sleep(3);
    level++;
    missiles_left = level * 10;
    delay = 50 - level;

    /* bonus city on odd levels if there is room */
    if (level % 2) {
      city_x[num_cities] = random() % (WIDTH - 50) + 25;
      if (find_city(city_x[num_cities], 50) < 0)
        num_cities++;
    }

    XClearWindow(display, window);
    for (i = 0; i < num_cities; i++)
      draw_city(i);

    for (;;) {
      alive = draw_objects();
      if (missiles_left == 0 && alive == 0)
        break;

      if (missiles_left && random() % delay == 0) {
        missiles_left--;
        type = random() % delay == 0 ? SMART_BOMB : MISSILE;
        start_x = random() % (WIDTH << FIX);
        target_x = random() % (WIDTH << FIX);
        launch(type, start_x, 0, target_x, HEIGHT << FIX, 1, 0);
      }

      usleep(delay * 200);

      /* each ABM costs one point */
      if (XCheckMaskEvent(display, ButtonPressMask, &event) && num_cities &&
          --score)
        launch(ABM, city_x[random() % MAX_CITIES] << FIX,
               (HEIGHT - 10) << FIX, event.xbutton.x << FIX,
               event.xbutton.y << FIX, 5, 0);
    }

    score += num_cities * 100;
    length = sprintf(message, score_format, level, score);
    XDrawString(display, window, gc, WIDTH / 3, HEIGHT / 2, message, length);
  }
}

int main() {
  long i;

  display = XOpenDisplay(0);
  window = RootWindow(display, 0);
  for (i = 0; i < 2; i++) {
    window = XCreateSimpleWindow(display, window, 0, 0, WIDTH, HEIGHT, 0, 0, 0);
    XMapWindow(display, window);
  }
  XSelectInput(display, window, ButtonPressMask | ExposureMask);
  XMaskEvent(display, ExposureMask, &event);
  gc = XCreateGC(display, window,
                 GCFunction | GCForeground | GCBackground | GCLineWidth |
                     GCLineStyle | GCFillStyle | GCFillRule,
                 &gc_values);
  srandom(time(0));
  play();
  puts(message);
}
