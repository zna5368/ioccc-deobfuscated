#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <GL/glx.h>
#include <GL/glu.h>

/*
 * The original program read this data out of its own source file: the
 * texture came from the whitespace inside the brace art of function Y() and
 * the cube geometry from the brace art of function Q().  Every second
 * character was sampled; whitespace meant grey (or 1) and braces/semicolons
 * meant red (or 0).  The decoded data is stored directly here instead.
 */

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define TEXTURE_SIZE 16

/* '.' = whitespace in the original art (grey), '#' = brace (dark red) */
static const char *texture_art[TEXTURE_SIZE] = {
    "################",
    "................",
    ".##..###.###.###",
    "...#.#.#.#.#.#..",
    ".##..#.#.#.#..##",
    ".#...#.#.#.#...#",
    ".###.###.###.###",
    "................",
    "................",
    ".#.###..#..#..#.",
    ".#.#.#.#..#..#..",
    ".#.#.#.#..#..#..",
    ".#.#.#.#..#..#..",
    ".#.###..#..#..#.",
    "................",
    "################",
};

/* one face of the unit cube: a normal followed by four corners */
struct face {
    int normal[3];
    int corner[4][3];
};

/*
 * Only 0 and 1 could be encoded in the original art, so the three faces
 * that should point in a negative direction have a zero normal.
 */
static const struct face cube_faces[6] = {
    {{0, 0, 0}, {{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0}}}, /* z = 0 */
    {{1, 0, 0}, {{1, 1, 0}, {1, 1, 1}, {1, 0, 1}, {1, 0, 0}}}, /* x = 1 */
    {{0, 1, 0}, {{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}}}, /* y = 1 */
    {{0, 0, 0}, {{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}}}, /* x = 0 */
    {{0, 0, 0}, {{1, 0, 0}, {1, 0, 1}, {0, 0, 1}, {0, 0, 0}}}, /* y = 0 */
    {{0, 0, 1}, {{1, 0, 1}, {1, 1, 1}, {0, 1, 1}, {0, 0, 1}}}, /* z = 1 */
};

/* texture coordinates used for the four corners of every face */
static const int face_texcoord[4][2] = {{1, 1}, {1, 0}, {0, 0}, {0, 1}};

int main(int argc, char **argv)
{
    int visual_attributes[] = {GLX_DOUBLEBUFFER, GLX_RGBA, GLX_DEPTH_SIZE, 16,
                               None};
    /* light_color[0..3] is toggled by the space bar */
    GLfloat light_color[4] = {1, 1, 1, 1};
    GLfloat light_position[4] = {5, 5, 100, 0};
    GLfloat ambient[4] = {.6, .6, .6, 1};
    GLubyte texture[TEXTURE_SIZE * TEXTURE_SIZE * 4];
    GLfloat angle = 0;     /* rotation of both cubes in degrees */
    GLfloat distance = 10; /* how far the second cube is to the left */
    int toggle_index = 0;  /* next light_color component to toggle */
    Display *display;
    XVisualInfo *visual;
    XSetWindowAttributes attributes;
    GLXContext context;
    Window window;
    XEvent event;
    GLuint texture_id, cube_list;
    int x, y, face, corner;

    display = XOpenDisplay(NULL);
    if (!display) {
        printf("%i\n", 1); /* X problem */
        return 0;
    }
    if (!glXQueryExtension(display, NULL, NULL)) {
        printf("%i\n", 2); /* can't open GLX */
        return 0;
    }
    visual = glXChooseVisual(display, DefaultScreen(display),
                             visual_attributes);
    attributes.colormap =
        XCreateColormap(display, RootWindow(display, visual->screen),
                        visual->visual, AllocNone);
    context = glXCreateContext(display, visual, NULL, True);
    if (!context) {
        printf("%i\n", 3); /* can't open 16bpp visual */
        return 0;
    }
    attributes.event_mask = KeyPressMask;
    window = XCreateWindow(display, RootWindow(display, visual->screen),
                           WINDOW_WIDTH, WINDOW_HEIGHT,
                           WINDOW_WIDTH, WINDOW_HEIGHT, 0, visual->depth,
                           InputOutput, visual->visual,
                           CWColormap | CWEventMask, &attributes);
    glXMakeCurrent(display, window, context);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);

    /* build the RGBA texture: grey where the art is blank, dark red else */
    for (y = 0; y < TEXTURE_SIZE; y++) {
        for (x = 0; x < TEXTURE_SIZE; x++) {
            GLubyte *pixel = &texture[(y * TEXTURE_SIZE + x) * 4];
            int blank = texture_art[y][x] == '.';
            pixel[0] = 128;
            pixel[1] = blank ? 128 : 0;
            pixel[2] = blank ? 128 : 0;
            pixel[3] = 255;
        }
    }
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEXTURE_SIZE, TEXTURE_SIZE, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, texture);

    glMatrixMode(GL_PROJECTION);
    gluPerspective(60, 1.333, 1, 100);
    glMatrixMode(GL_MODELVIEW);

    /* compile the textured cube into a display list */
    cube_list = glGenLists(1);
    glNewList(cube_list, GL_COMPILE);
    glEnable(GL_TEXTURE_2D);
    for (face = 0; face < 6; face++) {
        const struct face *f = &cube_faces[face];
        glBegin(GL_QUADS);
        glNormal3f(f->normal[0], f->normal[1], f->normal[2]);
        for (corner = 0; corner < 4; corner++) {
            glTexCoord2i(face_texcoord[corner][0], face_texcoord[corner][1]);
            glVertex3i(f->corner[corner][0], f->corner[corner][1],
                       f->corner[corner][2]);
        }
        glEnd();
    }
    glEndList();

    XMapWindow(display, window);

    for (;;) {
        if (XPending(display)) {
            XNextEvent(display, &event);
            /* any key whose keysym has bit 5 set (such as space) */
            if (XLookupKeysym((XKeyEvent *)&event, 0) & 0x20) {
                distance = 10;
                light_color[toggle_index] = !light_color[toggle_index];
                toggle_index = (toggle_index + 1) % 4;
            } else {
                printf("%i\n", 0); /* no error */
                return 0;
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();
        gluLookAt(-2, 0, 5, 0, 0, 0, 0, 0, 1);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glLightfv(GL_LIGHT0, GL_POSITION, light_position);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, light_color);
        glLightfv(GL_LIGHT0, GL_SPECULAR, light_color);
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
        glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, ambient);

        /* a spinning cube at the origin ... */
        glRotatef(angle, 1, 1, 1);
        glCallList(cube_list);
        /* ... and a second one sliding along the rotated x axis */
        glTranslatef(-distance, 0, 0);
        glCallList(cube_list);

        usleep(4000);
        angle += 0.07 * 6; /* the original reused a loop counter (6) here */
        if (angle > 360)
            angle -= 360.0;
        distance -= 6 / 100.0;
        glFlush();
        glXSwapBuffers(display, window);
    }
}
