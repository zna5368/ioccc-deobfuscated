#include <X11/Xlib.h>
#include <stdlib.h>

#define MAX_PIECES 999
/* stop splitting once this many pieces exist (a split adds three more) */
#define SPLIT_LIMIT 997
#define DEFAULT_SPEED 9

/*
 * One moving piece of the "shattered" screen.  Each piece is a bordered
 * frame window that contains a full screen sized image window.  The image
 * window shows the screen snapshot and is shifted inside the frame so that
 * the frame shows the part of the screen it was cut out of.
 */
struct piece {
    long x, y;           /* position of the frame on the screen */
    long width, height;  /* size of the frame */
    long dx, dy;         /* velocity in pixels per step (times the speed) */
    long image_x;        /* offset of the screen image inside the frame */
    long image_y;
};

struct piece pieces[MAX_PIECES];
Window frames[MAX_PIECES];
int piece_count;

long screen_width, screen_height;
unsigned long black;
Display *display;
Window cover;     /* full screen window that hides the real desktop */
Pixmap snapshot;  /* copy of the screen taken at start up */

/*
 * Create a new piece from part of the parent piece.  The new piece starts
 * at (offset_x, offset_y) inside the parent, has the given size and gets
 * the parent's velocity plus (add_dx, add_dy).
 */
void make_piece(struct piece *parent, long width, long height,
                long offset_x, long offset_y, long add_dx, long add_dy)
{
    struct piece *child = &pieces[piece_count];
    Window frame, image;

    child->x = parent->x + offset_x;
    child->y = parent->y + offset_y;
    child->width = width;
    child->height = height;
    frame = XCreateSimpleWindow(display, cover, child->x, child->y,
                                width, height, 1, black, 0);
    frames[piece_count] = frame;

    child->image_x = parent->image_x - offset_x;
    child->image_y = parent->image_y - offset_y;
    image = XCreateSimpleWindow(display, frame, child->image_x, child->image_y,
                                screen_width, screen_height, 0, 0, 0);
    XSetWindowBackgroundPixmap(display, image, snapshot);

    child->dx = parent->dx + add_dx;
    child->dy = parent->dy + add_dy;

    XMapRaised(display, image);
    XMapRaised(display, frame);
    piece_count++;
}

/*
 * Bounce the piece off the edges of the screen.  Returns how many edges the
 * piece is beyond.  At most one direction is reversed per call: once a
 * velocity is flipped the remaining edges are not checked.
 */
int bounce(struct piece *p)
{
    int hits = 0;

    if (p->x < 0) {
        hits++;
        if (p->dx < 0) {
            p->dx = -p->dx;
            return hits;
        }
    }
    if (p->x + p->width - screen_width > 0) {
        hits++;
        if (p->dx > 0) {
            p->dx = -p->dx;
            return hits;
        }
    }
    if (p->y < 0) {
        hits++;
        if (p->dy < 0) {
            p->dy = -p->dy;
            return hits;
        }
    }
    if (p->y + p->height - screen_height > 0) {
        hits++;
        if (p->dy > 0) {
            p->dy = -p->dy;
            return hits;
        }
    }
    return hits;
}

/*
 * Split a piece into four quarters.  The piece itself shrinks to the top
 * left quarter and three new pieces fly off from it.
 */
void shatter(struct piece *p, Window frame, long half_width, long half_height)
{
    make_piece(p, p->width - half_width, p->height - half_height,
               half_width, half_height, 2, 2);          /* bottom right */
    make_piece(p, half_width, p->height - half_height,
               0, half_height, -2, -2);                 /* bottom left */
    make_piece(p, p->width - half_width, half_height,
               half_width, 0, 2, -2);                   /* top right */
    p->image_y -= 2;
    XResizeWindow(display, frame, p->width = half_width,
                  p->height = half_height);
    p->image_x -= 2;
}

main(argc, argv) char **argv;
{
    XSetWindowAttributes attributes;
    XGCValues gc_values;
    Window root;
    GC gc;
    int screen, speed, count, i;
    long half_width, half_height, energy;
    struct piece *p;
    struct piece whole_screen = {0};

    display = XOpenDisplay(0);
    screen = DefaultScreen(display);

    /* take a snapshot of everything currently on the screen */
    root = RootWindow(display, screen);
    screen_width = DisplayWidth(display, screen);
    screen_height = DisplayHeight(display, screen);
    snapshot = XCreatePixmap(display, root, screen_width, screen_height,
                             DefaultDepth(display, screen));
    gc_values.subwindow_mode = IncludeInferiors;
    gc = XCreateGC(display, root, GCSubwindowMode, &gc_values);
    XCopyArea(display, root, snapshot, gc, 0, 0, screen_width, screen_height,
              0, 0);

    /* cover the screen with a black window the window manager ignores */
    black = BlackPixel(display, screen);
    cover = XCreateSimpleWindow(display, root, 0, 0, screen_width,
                                screen_height, 0, 0, black);
    attributes.override_redirect = True;
    XChangeWindowAttributes(display, cover, CWOverrideRedirect, &attributes);
    XMapRaised(display, cover);

    /* the first piece is the whole screen moving diagonally */
    make_piece(&whole_screen, screen_width, screen_height, 0, 0, 3, 3);

    speed = argc != 1 ? atoi(argv[1]) : DEFAULT_SPEED;

    for (;;) {
        count = piece_count;
        for (i = 0; i < count; i++) {
            p = &pieces[i];
            p->x += p->dx * speed;
            p->y += p->dy * speed;
            XMoveWindow(display, frames[i], p->x, p->y);

            if (!bounce(p))
                continue;

            /* a piece hitting the edge may break if it is big enough */
            half_width = p->width / 2;
            half_height = p->height / 2;
            if (half_width == 0 || half_height == 0 ||
                piece_count >= SPLIT_LIMIT)
                continue;

            /* the chance to break grows with the kinetic energy */
            energy = p->width * p->height * (p->dx * p->dx + p->dy * p->dy);
            if (rand() % 32768 <= energy / 999)
                shatter(p, frames[i], half_width, half_height);
        }
    }
}
