#include <math.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

/*
 * Controls and the time step are given on the compiler command line
 * (see the Makefile): -DIT=XK_Page_Up -DDT=XK_Page_Down -DUP=XK_Up ...
 */
#define KEY_THROTTLE_UP IT
#define KEY_THROTTLE_DOWN DT
#define KEY_STICK_FORWARD UP
#define KEY_STICK_BACK DN
#define KEY_STICK_LEFT LT
#define KEY_STICK_RIGHT RT
#define KEY_CENTER_STICK CS
#define TIME_STEP dt

#define MAX_POINTS 999
#define WINDOW_SIZE 400
#define PEN_UP 10000          /* "no previous point" marker */
#define GRAVITY 32.2          /* ft/s^2 */
#define MASS 74.5             /* slugs */
#define FT_PER_SEC_PER_KNOT 1.7
#define DEGREES_PER_RADIAN 57.3
#define END_OF_FILE (-1)

/* scenery: line strips of 3D points, a point summing to 0 lifts the pen */
double point_x[MAX_POINTS], point_y[MAX_POINTS], point_z[MAX_POINTS];
int num_points;

/* aircraft position (altitude is positive upwards) */
double pos_x, pos_y, altitude = 1000;

/* velocity in the aircraft's own frame */
double vel_forward = 221, vel_side, vel_down = 7.26;

/* orientation (Euler angles) and their body rates */
double roll, pitch = 33e-3, heading;
double roll_rate, pitch_rate, yaw_rate;

/* accelerations carried over to the start of the next frame */
double side_accel, yaw_accel;

/* pilot controls */
double throttle = 8, elevator, aileron;

double time_step = TIME_STEP;
char hud[52];

int main() {
  Display *display = XOpenDisplay(0);
  Window window = RootWindow(display, 0);
  GC gc;

  gc = XCreateGC(display, window, 0, 0);
  XSetForeground(display, gc, BlackPixel(display, 0));

  /* read the scenery from standard input */
  while (scanf("%lf%lf%lf", &point_x[num_points], &point_y[num_points],
               &point_z[num_points]) != END_OF_FILE)
    num_points++;

  window = XCreateSimpleWindow(display, window, 0, 0, WINDOW_SIZE,
                               WINDOW_SIZE, 0, 0, WhitePixel(display, 0));
  XSelectInput(display, window, KeyPressMask);
  XMapWindow(display, window);

  for (;;) {
    struct timeval frame_delay = {0, TIME_STEP * 1e6};
    double cos_roll = cos(roll), sin_roll = sin(roll);
    double cos_pitch = cos(pitch), sin_pitch;
    double cos_heading = cos(heading), sin_heading = sin(heading);
    /* rotation matrix from aircraft frame to world frame */
    double r11, r12, r13, r21, r22, r23, r32, r33;
    double speed_sq, roll_damping, yaw_damping, pitch_damping, sideslip;
    double lift_coef, side_force, forward_accel, old_pitch_rate;
    int idx, prev_x = PEN_UP, prev_y = 0;

    vel_side += side_accel * time_step;
    r33 = cos_roll * cos_pitch;
    yaw_rate += time_step * yaw_accel;
    r32 = sin_roll * cos_pitch;
    r11 = cos_pitch * cos_heading;
    r21 = cos_pitch * sin_heading;
    heading += cos_roll * time_step * yaw_rate / cos_pitch +
               pitch_rate / cos_pitch * sin_roll * time_step;
    sin_pitch = sin(pitch);
    r23 = sin_pitch * sin_heading * cos_roll - sin_roll * cos_heading;
    XClearWindow(display, window);
    r13 = sin_heading * sin_roll + cos_roll * sin_pitch * cos_heading;
    pitch += pitch_rate * time_step * cos_roll -
             time_step * yaw_rate * sin_roll;
    r12 = cos_heading * sin_roll * sin_pitch - sin_heading * cos_roll;
    r22 = cos_roll * cos_heading + sin_roll * sin_heading * sin_pitch;
    roll += (sin_roll * pitch_rate / cos_pitch * sin_pitch + roll_rate +
             sin_pitch / cos_pitch * yaw_rate * cos_roll) *
            time_step;

    /* draw the scenery in perspective */
    for (idx = 0; idx < num_points; idx++) {
      double dz = point_z[idx] + altitude;
      double dy = point_y[idx] - pos_y;
      double dx = point_x[idx] - pos_x;
      double ahead = dx * r11 - sin_pitch * dz + r21 * dy;
      double right = dz * r32 + r22 * dy + dx * r12;
      double below = r13 * dx + r33 * dz + r23 * dy;

      if (point_x[idx] + point_y[idx] + point_z[idx] == 0 ||
          ahead < fabs(right) || fabs(below) > ahead) {
        prev_x = PEN_UP; /* break in the line, or outside the view */
      } else {
        int screen_x = right / ahead * 400 + 200;
        int screen_y = 200 + 400 / ahead * below;
        if (prev_x != PEN_UP)
          XDrawLine(display, window, gc, prev_x, prev_y, screen_x, screen_y);
        prev_x = screen_x;
        prev_y = screen_y;
      }
    }

    speed_sq = vel_down * vel_down + vel_forward * vel_forward +
               vel_side * vel_side;
    XDrawString(display, window, gc, 20, 380, hud, 17);
    roll_damping = roll_rate / vel_forward * 15;
    sideslip = vel_side / vel_forward;

    /* move the aircraft in the world */
    pos_x += time_step *
             (vel_down * r13 + r12 * vel_side + r11 * vel_forward);
    altitude += (sin_pitch * vel_forward - vel_side * r32 - vel_down * r33) *
                time_step;
    /* the original reuses sideslip here instead of r22 */
    pos_y += (vel_forward * r21 + sideslip * vel_side + r23 * vel_down) *
             time_step;

    /* keyboard: throttle and stick */
    while (XPending(display)) {
      XEvent event;
      KeySym key;
      XNextEvent(display, &event);
      key = XLookupKeysym(&event.xkey, 0);
      if (key == KEY_THROTTLE_UP)
        throttle++;
      else if (key == KEY_STICK_LEFT)
        aileron++;
      else if (key == KEY_STICK_FORWARD)
        elevator++;
      if (key == KEY_STICK_BACK)
        elevator--;
      else if (key == KEY_THROTTLE_DOWN)
        throttle--;
      else if (key == KEY_STICK_RIGHT)
        aileron--;
      if (key == KEY_CENTER_STICK)
        aileron = 0;
    }

    /* aerodynamics */
    yaw_damping = 15 * yaw_rate / vel_forward;
    lift_coef = .1 + vel_down * 4.9 / vel_forward;
    side_force = speed_sq * yaw_damping / 32 - sideslip * speed_sq / 24;
    side_accel = GRAVITY * r32 + roll_rate * vel_down -
                 yaw_rate * vel_forward + side_force / MASS;
    forward_accel =
        yaw_rate * vel_side +
        (throttle * 1e4 / vel_forward -
         (speed_sq + lift_coef * 5 * speed_sq * lift_coef) / 3e2) /
            MASS -
        vel_down * pitch_rate - sin_pitch * GRAVITY;
    pitch_damping = 2.63 / vel_forward * pitch_rate;
    vel_down += (pitch_rate * vel_forward -
                 speed_sq / MASS *
                     (.19 * lift_coef + pitch_damping * .64 + elevator / 1e3) -
                 vel_side * roll_rate + GRAVITY * r33) *
                time_step;
    vel_forward += forward_accel * time_step;
    old_pitch_rate = pitch_rate;

    /* heads up display: airspeed (knots), heading (degrees), altitude (ft) */
    sprintf(hud, "%5d  %3d%7d", (int)(vel_forward / FT_PER_SEC_PER_KNOT),
            (int)(9000 + heading * DEGREES_PER_RADIAN) % 360, (int)altitude);

    pitch_rate += speed_sq *
                      (.45 - 14 / vel_forward * vel_down -
                       pitch_damping * 130 - elevator * .14) *
                      time_step / 125e2 +
                  yaw_rate * time_step * roll_rate;
    yaw_accel = (speed_sq *
                     (47 * sideslip - yaw_damping * 52 +
                      lift_coef * 94 * roll_damping - side_force * .38 +
                      aileron * .21 * lift_coef) /
                     1e2 +
                 old_pitch_rate * 179 * roll_rate) /
                2312;

    select(0, 0, 0, 0, &frame_delay);

    roll_rate -= (old_pitch_rate * yaw_rate -
                  speed_sq *
                      (.63 * yaw_damping - sideslip * .086 +
                       yaw_damping * lift_coef * 19 - roll_damping * 25 -
                       .11 * aileron) /
                      107e2) *
                 time_step;
  }
}
