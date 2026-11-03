#include <X11/Xlib.h>

/*
 * Interactive fractals.  The left window shows a static "Mandelbrot-like"
 * picture; moving the mouse over it picks the parameter c for the animated
 * "Julia-like" picture in the right window.
 *
 * Every pixel p of a window stands for a point z.  next_index[p] holds the
 * pixel index of the transformed point (without the parameter c).  Adding
 * c_offset (the pixel offset of the mouse from the window centre) applies
 * "+ c", so one iteration of the fractal formula is a single table lookup:
 *
 *     next pixel = next_index[pixel] + c_offset
 *
 * Any result outside 0..pixels-1 (as unsigned) means the point escaped.
 */

void *calloc();

Display *display;
int *next_index;        /* fractal map, one entry per pixel                */
int *orbit_stack;       /* pixels visited while following an orbit         */
int pixels;             /* number of pixels in a window (size * size)      */
int c_offset;           /* mouse position as an offset from the centre     */
int center_index;       /* pixel index of the point z = 0                  */
int half;               /* half of the window width                        */
int size;               /* window width and height                         */
int ncolors;            /* number of colours read from the colour map file */
int max_count;          /* highest iteration count (ncolors - 1)           */
int stable_flag;        /* < 0 means: draw the stable picture next         */
unsigned char *prev_count; /* iteration count per pixel, previous frame    */
unsigned char *cur_count;  /* iteration count per pixel, current frame     */
unsigned char *buffer_end; /* end of the block of buffers                  */
XPoint *pixel_points;   /* window coordinate of each pixel                 */
XPoint *draw_points;    /* changed pixels sorted by colour                 */
Window julia_window;
Window draw_window;
GC gc[256];             /* one graphics context per colour                 */
int histogram[256];     /* number of pixels having each count              */
int mode;
int screen;

void create_window(int event_mask) {
  /* The first window is placed at x = size, the second at x = 0 */
  draw_window = XCreateSimpleWindow(
      display, XRootWindow(display, screen = XDefaultScreen(display)),
      draw_window ? 0 : size, 0, size, size, 0, 0, 0);
  XMapWindow(display, draw_window);
  XSelectInput(display, draw_window, event_mask);
}

/*
 * Allocate all buffers as one block laid out as
 *   cur_count[pixels], prev_count[pixels], draw_points[pixels],
 *   pixel_points[pixels], orbit_stack[pixels], next_index[pixels].
 * The block size is taken from the previous layout, so it is called three
 * times: first a 256 byte line buffer (pixels is still 0), then once more to
 * measure the real layout, and finally to allocate it.
 */
void allocate_buffers(void) {
  cur_count = calloc(buffer_end - cur_count + 256, 1);
  prev_count = cur_count + pixels;
  draw_points = (XPoint *)(prev_count + pixels);
  pixel_points = draw_points + pixels;
  orbit_stack = (int *)(pixel_points + pixels);
  next_index = orbit_stack + pixels;
  buffer_end = (unsigned char *)(next_index + pixels);
}

/* Fill next_index with the fractal formula and pixel_points with coordinates */
void build_map(void) {
  int fx, fy, pixel;
  unsigned formula = mode % 4;

  fx = fy = half;
  for (pixel = 0; pixel < pixels; pixel++) {
    switch (formula) {
    case 0: /* Mandelbrot / Julia: z*z */
      next_index[pixel] = center_index + 2 * (fx * fx - fy * fy) / half +
                          4 * fx * fy / half * size;
      break;
    case 1: /* variation of the above */
      next_index[pixel] = center_index + (fy * fy - fx * fx) / half * size +
                          2 * fx * fy / half;
      break;
    case 2: /* swap the coordinates: 3D-look-a-like */
      next_index[pixel] = center_index + fy + fx * size;
      break;
    default: /* variation of the above */
      next_index[pixel] = center_index + (fy - fx * fx / size) + fx * size;
      break;
    }
    pixel_points[pixel].x = half - fx;
    pixel_points[pixel].y = half - fy;
    fx--;
    if (fx + half == 0) {
      fx = half;
      fy--;
    }
  }
}

/*
 * Non-stable mode: the count of a pixel is the count of the pixel it moves
 * to in the previous frame plus one, so the picture converges over time.
 */
void iterate_unstable(void) {
  int pixel, target;

  for (pixel = 0; pixel < pixels; pixel++) {
    target = next_index[pixel] + c_offset;
    if ((unsigned)target < pixels)
      cur_count[pixel] = prev_count[target] + (prev_count[target] < max_count);
    else
      cur_count[pixel] = 1;
    histogram[cur_count[pixel]]++;
  }
}

/* Give the pixels on orbit_stack counts growing from value, last one first */
void assign_orbit(int depth, int value) {
  while (depth) {
    value += value < max_count;
    cur_count[orbit_stack[--depth]] = value;
    histogram[value]++;
  }
}

/*
 * Non-stable mode with attractor searcher: as above, but also follow the
 * orbit of the critical point z = 0 and recount the pixels along it.
 */
void iterate_with_attractor_search(void) {
  int value = 0, depth = 0;
  unsigned pos;

  iterate_unstable();
  for (pos = c_offset + center_index;
       pos < pixels && (value = cur_count[pos]) < max_count;
       pos = next_index[pos] + c_offset) {
    histogram[value]--;
    cur_count[orbit_stack[depth++] = pos] = max_count;
  }
  if (pos >= pixels)
    value = 1;
  assign_orbit(depth, value);
}

/*
 * Stable mode: follow the orbit of every uncounted pixel until it escapes or
 * reaches a pixel with a known count, then count backwards along the orbit.
 * Pixels on the current orbit are marked with max_count so cycles stop.
 */
void iterate_stable(void) {
  int pixel, value;
  unsigned start, pos;
  int depth = 0;

  for (pixel = 0; pixel < pixels; pixel++)
    cur_count[pixel] = 0;
  stable_flag = -1;
  for (start = 0; start < pixels; start++) {
    for (pos = start; pos < pixels && !(value = cur_count[pos]);
         pos = next_index[pos] + c_offset)
      cur_count[orbit_stack[depth++] = pos] = max_count;
    if (pos >= pixels)
      value = 0;
    assign_orbit(depth, value);
    depth = 0;
  }
}

/* Draw the pixels whose count changed and handle the pending X events */
void draw_frame(void) {
  XPoint *bucket[256], *start;
  int color;
  XEvent event;

  /* Bucket sort the changed pixels by colour */
  start = draw_points;
  for (color = 0; color < ncolors; color++) {
    bucket[color] = start;
    start += histogram[color];
  }
  for (color = 0; color < pixels; color++)
    if (prev_count[color] != cur_count[color])
      *bucket[cur_count[color]]++ = pixel_points[color];

  start = draw_points;
  for (color = 0; color < ncolors; color++) {
    XDrawPoints(display, draw_window, gc[color], start, bucket[color] - start,
                CoordModeOrigin);
    start += histogram[color];
    histogram[color] = 0;
  }

  /* After a stable picture (stable_flag reaches 0) wait for an event */
  for (;;) {
    stable_flag++;
    if (!XPending(display) && stable_flag)
      break;
    XNextEvent(display, &event);
    if (event.type == ButtonPress)
      stable_flag = -42;
    else
      c_offset = event.xmotion.x + event.xmotion.y * size - center_index;
  }
}

int parse_number(char *text) {
  if (!text) {
    text = (char *)cur_count;
    fgets(text, buffer_end - cur_count + 256, stdin);
    text[strlen(text) - 1] = '\0';
  }
  return atoi(text);
}

void main(int argc, char *argv[]) {
  int pixel, count, offset, method;
  unsigned pos;
  unsigned char *swap;

  if (argc < 3) {
    puts("tvr: mode size < cmap");
    exit(0);
  }
  allocate_buffers();
  mode = parse_number(argv[1]) % 12;
  half = parse_number(argv[2]);
  size = half * 2;
  pixels = size * size;
  allocate_buffers();
  center_index = pixels / 2 + half;

  display = XOpenDisplay(0);
  create_window(0);
  julia_window = draw_window;
  create_window(PointerMotionMask | ButtonPressMask);

  ncolors = parse_number(0);
  max_count = ncolors - 1;
  {
    XColor color;
    Colormap colormap;
    for (pixel = 0; pixel < ncolors; pixel++) {
      parse_number(0);
      XParseColor(display, colormap = XDefaultColormap(display, screen),
                  (char *)cur_count, &color);
      XAllocColor(display, colormap, &color);
      XSetForeground(display,
                     gc[pixel] = XCreateGC(display, julia_window, 0, 0),
                     color.pixel);
    }
  }

  allocate_buffers();
  build_map();

  /* Static picture: iterate z from 0 with c = point of each pixel */
  pos = 0;
  offset = 0;
  for (pixel = 0; pixel < pixels; pixel++) {
    count = 0;
    do
      count++;
    while (count < max_count && (pos = next_index[pos] + offset) < pixels);
    histogram[cur_count[pixel] = count]++;
    pos = center_index;
    offset = pixel + 1 - center_index;
  }
  draw_frame();

  /* Animate the other window */
  draw_window = julia_window;
  method = mode / 4;
  for (;;) {
    if (stable_flag < 0 || method == 2)
      iterate_stable();
    else if (method == 0)
      iterate_with_attractor_search();
    else
      iterate_unstable();
    draw_frame();
    swap = prev_count;
    prev_count = cur_count;
    cur_count = swap;
  }
}
