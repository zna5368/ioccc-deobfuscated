/*
 * Convex polyhedron viewer.
 *
 * Standard input is scanned for numbers, and every three numbers make one point
 * in space. Anything that is not a number is skipped, so a C source file can be
 * used as input if it holds enough numbers. The convex hull of the points is
 * found by gift wrapping, and the hull is then drawn with ASCII line art from a
 * camera that spirals around it forever.
 */

/* Screen size as "columns,rows". It can be replaced with -DS=columns,rows. */
#ifndef S
#define S 70,23
#endif

#define _POSIX_C_SOURCE 199309
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

/* Marks a vertex that has no next vertex in a face, or a missing argument. */
#define NONE (-1)

/* Two triangles lie in one plane if their signed volume is less than this. */
#define COPLANAR_VOLUME 1E-4

/* The random nudge given to every coordinate is at most 1 / JITTER_DIVISOR. */
#define JITTER_DIVISOR 1E5

/* The camera moves by at most 1 / SPIN_DIVISOR half turns each frame. */
#define SPIN_DIVISOR 1E3

/* Distance from the camera to the center. The points are scaled to radius 1. */
#define CAMERA_DISTANCE 9

/* For this long after starting, only the corners of every face are drawn. */
#define INTRO_SECONDS 3

/* Pause after each frame. */
#define FRAME_DELAY_NANOSECONDS 1000000

/* Number of series terms, and of square root steps, in the math functions. */
#define MATH_ITERATIONS 99

/* The value of pi that the sine and cosine series use. */
#define APPROXIMATE_PI 3.14

#define CLEAR_SCREEN "\x1b[2J"
#define CURSOR_HOME "\x1b[0H"
#define CURSOR_HOME_LENGTH 4

/*
 * Replacement for a character cell that has one more dot drawn in it. A cell is
 * two dots tall: ' is the top dot, , is the bottom dot and ; is both. The index
 * is the cell's current character modulo 8, with the lowest bit flipped if the
 * new dot is in the bottom half. This gives the following indexes:
 *
 *     current    top dot    bottom dot
 *     space      0          1
 *     '          7          6
 *     ,          4          5
 *     ;          3          2
 */
#define DOT_TABLE "',;;;,;'"

/* Where the camera is and which way its view points. */
struct camera {
	double position[3];
	double eye[3];      /* The position in the previous frame. */
	double right[3];    /* Sideways axis of the view. */
	double up[3];       /* Upwards axis of the view. */
	double speed;       /* Distance moved since the previous frame. */
};

static int width;   /* Screen width in characters. */
static int height;  /* Screen height in dots, which is two per character. */
static char *frame; /* The cursor home sequence and then the screen contents. */

static int vertex_count;
static double *points;   /* The scaled input points, three values for each. */
static double *jittered; /* The same points with some noise added. */
static double *corners;  /* Screen x and y of each corner of a face. */

/*
 * faces[i] is a polygon stored as a "next vertex" table, so faces[i][v] is the
 * vertex after v going around the polygon, or NONE if v is not a corner.
 * triangles[i] is the triangle that faces[i] started as. It is never changed,
 * which keeps a record of each directed edge that has been wrapped. When two
 * faces merge, each faces[i] that held either of them points to the new
 * polygon, so the same polygon can appear several times in the list.
 */
static int **faces;
static int **triangles;

/* Sets the cosine and sine of x half turns, using their Taylor series. */
static void pi_cos_sin(double x, double *cosine, double *sine)
{
	double term = 1;
	int power = 0;

	*cosine = *sine = 0;
	while (power < MATH_ITERATIONS) {
		if (power % 2) {
			*sine += term;
			term = -term;
		} else {
			*cosine += term;
		}
		power++;
		term *= APPROXIMATE_PI * x / power;
	}
}

/* Square root by Newton's method. */
static double newton_sqrt(double x)
{
	double root = 1;
	int step;

	for (step = 0; step < MATH_ITERATIONS; step++)
		root = (root + x / root) / 2;
	return root;
}

/* Random number from -1 to 1. */
static double random_unit(void)
{
	return rand() * 2. / RAND_MAX - 1;
}

/* One coordinate of the vector from jittered vertex "from" to vertex "to". */
static double offset(int from, int to, int axis)
{
	return jittered[to * 3 + axis] - jittered[from * 3 + axis];
}

/* Z part of the cross product of the vectors from vertex p to q and to r. */
static double cross_z(int p, int q, int r)
{
	return offset(p, q, 0) * offset(p, r, 1)
		- offset(p, q, 1) * offset(p, r, 0);
}

/*
 * Six times the signed volume of the tetrahedron p, q, r, s. This is the
 * determinant of the three vectors from p, written out in full. It is negative
 * when s is on one side of the plane through p, q and r, and positive on the
 * other.
 */
static double signed_volume(int p, int q, int r, int s)
{
	return cross_z(p, q, r) * offset(p, s, 2)
		+ cross_z(p, r, s) * offset(p, q, 2)
		+ cross_z(p, s, q) * offset(p, r, 2);
}

/*
 * Joins two polygons that share an edge into the table merged. Edges that are
 * in both polygons run in opposite directions, and are dropped. A vertex that
 * is left with no edges at all was inside the new polygon and is removed.
 */
static void merge_faces(int *merged, int *first, int *second, int face_limit)
{
	int vertex, face;

	for (vertex = 0; vertex < vertex_count; vertex++) {
		int first_next = first[vertex];
		int second_next = second[vertex];

		if (second_next == NONE)
			merged[vertex] = first_next;
		else if (first[second_next] != vertex)
			merged[vertex] = second_next;
		else if (second[first_next] != vertex)
			merged[vertex] = first_next;
		else
			merged[vertex] = NONE;
	}
	for (face = 0; face < face_limit; face++)
		if (faces[face] == first || faces[face] == second)
			faces[face] = merged;
}

/*
 * Gift wrapping step: finds the hull triangle on the directed edge from -> to,
 * then wraps the other two sides of that triangle in the same way.
 *
 * The edge belongs to the triangle in face slot neighbor, which runs along it
 * in the other direction and has opposite as its third vertex. Both are NONE
 * for the first edge. If the edge has been wrapped already then its two
 * triangles only need merging, which is done if they lie in the same plane.
 *
 * Returns the next free face slot, starting from free_slot.
 */
static int wrap_edge(int from, int to, int opposite, int neighbor,
		int free_slot)
{
	int *next = malloc(sizeof(int) * vertex_count);
	int own_slot = free_slot;
	int third = NONE;
	int slot, vertex;

	faces[own_slot] = triangles[own_slot] = next;

	for (slot = 0; slot < own_slot; slot++) {
		if (triangles[slot][from] == to) {
			if (opposite != NONE
					&& signed_volume(from, to,
						triangles[slot][to], opposite)
					< COPLANAR_VOLUME)
				merge_faces(next, faces[neighbor], faces[slot],
						own_slot);
			return own_slot;
		}
	}

	/* The third vertex is the one that leaves no vertex behind the plane. */
	for (vertex = 0; vertex < vertex_count; vertex++) {
		if (vertex != from && vertex != to
				&& (third == NONE || signed_volume(from, to,
						third, vertex) < 0))
			third = vertex;
		next[vertex] = NONE;
	}
	next[from] = to;
	next[to] = third;
	next[third] = from;

	/*
	 * Wrapping from -> to again finds this new triangle straight away, so
	 * the first call only merges it with its neighbor.
	 */
	free_slot = wrap_edge(from, to, opposite, neighbor, own_slot + 1);
	free_slot = wrap_edge(from, third, to, own_slot, free_slot);
	return wrap_edge(third, to, from, own_slot, free_slot);
}

/*
 * Reads the points, and scales them so that the furthest from the origin is at
 * distance 1. Input that cannot be read as a number is skipped one word at a
 * time. A number may be followed by a comma.
 */
static void read_points(void)
{
	double squared_length = 0;
	double longest = 0;
	int value_count = 0;
	int result;
	int i;

	for (;;) {
		double *value;

		points = realloc(points, (value_count + 1) * sizeof(double));
		value = points + value_count;
		result = scanf("%lf,", value);
		if (result < 0)
			break;
		if (result == 0) {
			scanf("%*s");
			continue;
		}
		squared_length += *value * *value;
		if (value_count++ % 3 == 2) {
			double length = newton_sqrt(squared_length);

			if (longest < length)
				longest = length;
			squared_length = 0;
		}
	}
	for (i = 0; i < value_count; i++)
		points[i] /= longest;
	vertex_count = value_count / 3;
}

/*
 * Moves every point by a tiny random amount. Exact ties, such as four corners
 * of a square lying in one plane, would otherwise confuse the gift wrapping.
 * The real faces are recovered by merging triangles that are almost in one
 * plane.
 */
static void jitter_points(void)
{
	int i;

	for (i = 0; i < 3 * vertex_count; i++)
		jittered[i] = points[i] + random_unit() / JITTER_DIVISOR;
}

/*
 * Finds an edge that must be on the hull. Seen from above, the vertex with the
 * lowest x is on the outline of the shape, and the next vertex around the
 * outline is found by keeping whichever vertex is furthest to one side.
 */
static void find_first_edge(int *from, int *to)
{
	int start = 0;
	int end;
	int vertex;

	for (vertex = 0; vertex < vertex_count; vertex++)
		if (jittered[start * 3] > jittered[vertex * 3])
			start = vertex;

	end = start == 0 ? 1 : 0;
	for (vertex = 0; vertex < vertex_count; vertex++)
		if (cross_z(start, end, vertex) > 0)
			end = vertex;

	*from = start;
	*to = end;
}

/* Sets result to the cross product of a and b. */
static void cross(const double *a, const double *b, double *result)
{
	result[0] = a[1] * b[2] - a[2] * b[1];
	result[1] = a[2] * b[0] - a[0] * b[2];
	result[2] = a[0] * b[1] - a[1] * b[0];
}

/* Keeps an angle measured in half turns between -1 and 1. */
static double wrap_angle(double angle)
{
	if (angle < -1)
		return angle + 2;
	if (angle > 1)
		return angle - 2;
	return angle;
}

/*
 * Places the camera on a sphere around the origin and works out the view axes.
 * The camera looks along the line through the origin, and the view is turned so
 * that the camera is always travelling towards the top of the screen.
 */
static void aim_camera(struct camera *camera, double polar, double azimuth)
{
	double polar_cos, polar_sin, azimuth_cos, azimuth_sin;
	double ring_radius;
	double motion[3];
	int axis;

	pi_cos_sin(polar, &polar_cos, &polar_sin);
	ring_radius = CAMERA_DISTANCE * polar_sin;
	pi_cos_sin(azimuth, &azimuth_cos, &azimuth_sin);

	camera->position[0] = ring_radius * azimuth_sin;
	camera->position[1] = ring_radius * azimuth_cos;
	camera->position[2] = CAMERA_DISTANCE * polar_cos;

	for (axis = 0; axis < 3; axis++)
		motion[axis] = camera->position[axis] - camera->eye[axis];
	camera->speed = newton_sqrt(motion[0] * motion[0]
			+ motion[1] * motion[1] + motion[2] * motion[2]);

	cross(motion, camera->position, camera->right);
	cross(camera->position, camera->right, camera->up);
}

/* Measures a point, as seen from the eye, along one of the view axes. */
static double view_distance(const struct camera *camera, const double *point,
		const double *axis)
{
	return (point[0] - camera->eye[0]) * axis[0]
		+ (point[1] - camera->eye[1]) * axis[1]
		+ (point[2] - camera->eye[2]) * axis[2];
}

/*
 * Perspective projection of a polygon into corners. The length of the view axes
 * depends on the speed of the camera, and dividing by the speed cancels this.
 *
 * Returns the number of values stored, which is two for each corner.
 */
static int project_face(const int *next, const struct camera *camera)
{
	int count = 0;
	int first = 0;
	int vertex;

	while (next[first] == NONE)
		first++;

	vertex = first;
	do {
		const double *point = points + next[vertex] * 3;
		double depth = view_distance(camera, point, camera->position);
		double across = view_distance(camera, point, camera->right);
		double up = view_distance(camera, point, camera->up);

		corners[count] = width / 2
			- across / camera->speed / depth * 3 * width;
		corners[count + 1] = height / 2
			+ up / camera->speed / depth * height / 2;
		count += 2;
		vertex = next[vertex];
	} while (vertex != first);

	return count;
}

/* The shoelace formula. The sign says which way around the polygon runs. */
static double polygon_area(int count)
{
	double area = 0;
	int i;

	for (i = 0; i < count; i += 2)
		area += corners[i] * (corners[(i + 3) % count]
				- corners[(i + count - 1) % count]);
	return area;
}

/* Blanks the screen contents. */
static void clear_frame(void)
{
	int end = width * height / 2 + CURSOR_HOME_LENGTH - 1;
	int i;

	for (i = CURSOR_HOME_LENGTH; i < end; i++)
		frame[i] = i % width == CURSOR_HOME_LENGTH - 1 ? '\n' : ' ';
	frame[end] = '\0';
}

/* Adds one dot to the screen if it is inside the edges. */
static void plot(int x, int y)
{
	if (0 <= x && x < width - 1 && 0 <= y && y < height) {
		char *cell = frame + y / 2 * width + x + CURSOR_HOME_LENGTH;

		*cell = DOT_TABLE[*cell % 8 ^ y % 2];
	}
}

/* Swaps two values. */
static void swap(double *a, double *b)
{
	double saved = *a;

	*a = *b;
	*b = saved;
}

/*
 * Draws a line by stepping one dot at a time along its longer direction. If the
 * line is steeper than it is wide, then x and y trade places first and trade
 * back as each dot is plotted. With corners_only set, the steps jump from the
 * first dot straight to the last one.
 */
static void draw_line(double x0, double y0, double x1, double y1,
		int corners_only)
{
	double rise = y1 - y0;
	int transposed = 0;
	int along;

	if (rise * rise > (x1 - x0) * (x1 - x0)) {
		swap(&x0, &y0);
		swap(&x1, &y1);
		/*
		 * The original uses the value of its last swap assignment as
		 * this flag, and not simply 1. It is the old x1 cut down to an
		 * int, so a steep line that ends at -1 < x < 1 is not trading
		 * back and gets plotted mirrored. Only a very narrow screen puts
		 * a corner there, and it is kept to match the original.
		 */
		transposed = (int)y1;
	}
	if (x1 - x0 < 0) {
		swap(&x0, &x1);
		swap(&y0, &y1);
	}

	along = (int)(x0 + .5);
	while (x1 - x0 != 0 && along < x1 + .5) {
		int across = (int)((y1 - y0) * (along - x0) / (x1 - x0)
				+ y0 + 0.5);

		if (transposed)
			plot(across, along);
		else
			plot(along, across);

		along++;
		if (along < x1 && corners_only)
			along = (int)(x1 + 0.5);
	}
}

/*
 * Draws the edges of one face. Faces pointing away from the camera are left
 * out, except during the intro when the corners of all faces are shown.
 */
static void draw_face(const int *next, const struct camera *camera,
		int start_time)
{
	int count = project_face(next, camera);
	double area = polygon_area(count);
	int intro = time(0) < start_time + INTRO_SECONDS;
	int i;

	if (intro || area >= 0)
		for (i = count; i > 0; i -= 2)
			draw_line(corners[i % count], corners[i % count + 1],
					corners[i - 2], corners[i - 1], intro);
}

int main(void)
{
	static struct camera camera;
	const int screen_size[2] = { S };
	struct timespec frame_delay = { 0, FRAME_DELAY_NANOSECONDS };
	double spin_cos, spin_sin;
	double polar_speed, azimuth_speed;
	double polar, azimuth;
	int start_time;
	int face_count;
	int first_from, first_to;
	int face, axis;

	width = screen_size[0];
	height = screen_size[1] * 2;
	frame = malloc(sizeof(char) * (height * width + CURSOR_HOME_LENGTH));
	sprintf(frame, CURSOR_HOME);

	start_time = (int)time(0);
	srand(start_time);

	read_points();
	jittered = malloc(sizeof(double) * vertex_count * 3);
	corners = malloc(sizeof(double) * vertex_count * 2);

	/* A random direction to spin in, and a random place to start from. */
	pi_cos_sin(random_unit(), &spin_cos, &spin_sin);
	polar_speed = spin_cos / SPIN_DIVISOR;
	azimuth_speed = spin_sin / SPIN_DIVISOR;
	polar = random_unit();
	azimuth = random_unit();

	/* A hull has fewer than two faces for each vertex. */
	faces = malloc(sizeof(int *) * 2 * vertex_count);
	triangles = malloc(sizeof(int *) * 2 * vertex_count);

	jitter_points();
	find_first_edge(&first_from, &first_to);

	puts(CLEAR_SCREEN);
	face_count = wrap_edge(first_from, first_to, NONE, NONE, 0);

	for (;;) {
		polar = wrap_angle(polar);
		azimuth = wrap_angle(azimuth);
		aim_camera(&camera, polar, azimuth);

		clear_frame();
		for (face = 0; face < face_count; face++)
			draw_face(faces[face], &camera, start_time);
		puts(frame);
		nanosleep(&frame_delay, 0);

		for (axis = 0; axis < 3; axis++)
			camera.eye[axis] = camera.position[axis];
		polar += polar_speed;
		azimuth += azimuth_speed;
	}
}
