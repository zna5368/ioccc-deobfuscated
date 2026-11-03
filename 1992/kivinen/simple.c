#include <X11/Xlib.h>

/*
 * Every game object is its own X window inside the playfield window.
 * Coordinates are in "game units" which are multiplied by the scale.
 */
#define MOVE(n, px, py) XMoveWindow(display, win[n], (px) * scale, (py) * scale)

#define PLAYER 1     /* paddle (breakout/dropout) or ship (invaders) */
#define FIRST_TARGET 2 /* first brick or alien window */
#define SHOT 32      /* missile (invaders) or falling piece (dropout) */
#define BALL 52      /* the breakout ball */

int main(argc, argv)
int argc;
char **argv;
{
  Window win[53];
  int n;           /* window index */
  int shot_y;      /* y of missile / falling piece / ball */
  int shot_x;      /* x of missile / falling piece / ball */
  int remaining;   /* targets left: aliens, bricks or pieces to catch */
  int player_x;    /* x of paddle or ship */
  int edge_col;    /* invaders: outermost alive column in moving direction */
  int hit = 0;     /* breakout/dropout: something touched the paddle/brick */
  int fleet_x;     /* invaders: horizontal offset of the alien fleet */
  int fleet_y = 32; /* invaders: vertical offset of the fleet; bit 5 set
                       means moving right, low 5 bits count a descent */
  int player_dx;   /* paddle speed set by mouse buttons (never initialized) */
  int col;         /* invaders: alien column */
  int ball_dx;     /* breakout: horizontal ball speed */
  int ball_dy = -1; /* breakout: vertical ball speed */
  int scale = (argc + 1) / 2;
  int falling = argc & 1; /* invaders and dropout: things fall/fly vertically */
  int invaders = falling && (scale & 1);
  int dropout = falling && !invaders;
  int breakout = !falling;
  int last = falling ? 32 : 52; /* index of the last window */
  int check;
  XEvent ev;
  Display *display = XOpenDisplay("");

  /* the playfield; it receives the mouse button events */
  win[0] = XCreateSimpleWindow(display, DefaultRootWindow(display), 200, 200,
                               (invaders ? 330 : 120) * scale, 215 * scale, 2,
                               BlackPixel(display, DefaultScreen(display)),
                               BlackPixel(display, DefaultScreen(display)));
  XMapWindow(display, win[0]);
  XSelectInput(display, win[0], ButtonPressMask | ButtonReleaseMask);

  /*
   * Create the player, the targets and the projectile. Every one of them
   * reports when it becomes partially covered by another window: this is
   * how collisions are detected.
   */
  for (n = 0; n < last; n++) {
    int px, py, width, height;
    if (falling) {
      /* lined up above the top edge; the aliens are moved in place later */
      px = n * 20;
      py = -20;
      width = n == SHOT - 1 ? 2 : 10;
      height = invaders ? 10 : 2;
    } else {
      /* 10 x 5 wall of bricks; n == 0 is the wide flat paddle */
      px = (n - 1) % 10 * 12;
      py = (n - 1) / 10 * 12;
      width = n ? 10 : 20;
      height = n ? 10 : 1;
    }
    ball_dx = fleet_x = 1;
    win[n + 1] = XCreateSimpleWindow(
        display, win[0], px * scale, py * scale, width * scale,
        height * scale, 1, BlackPixel(display, DefaultScreen(display)),
        WhitePixel(display, DefaultScreen(display)));
    XMapWindow(display, win[n + 1]);
    XSelectInput(display, win[n + 1], VisibilityChangeMask);
  }

  XMapWindow(display, win[PLAYER]);
  player_x = invaders ? 165 : 50;
  MOVE(PLAYER, player_x, falling ? 130 : 60);

  /* the projectile window (missile, falling piece or ball) */
  XMapWindow(display, win[last]);
  remaining = last - 2;
  if (falling) {
    shot_y = -20;
    MOVE(last, -10, shot_y);
  } else {
    shot_x = player_x;
    shot_y = 110;
    MOVE(last, shot_x, shot_y);
  }

  while (remaining && fleet_y < 160) {
    if (falling) {
      if (invaders) {
        /* move the fleet, remembering the outermost living column */
        edge_col = (fleet_y & 32) ? 0 : 9;
        for (n = 30; n--;) {
          if (win[n + FIRST_TARGET]) {
            col = n % 10;
            if (fleet_y & 32)
              edge_col = col > edge_col ? col : edge_col;
            else
              edge_col = col < edge_col ? col : edge_col;
            MOVE(n + FIRST_TARGET, 10 + col * 20 + fleet_x,
                 (n / 10) * 20 + fleet_y);
          }
        }
        edge_col *= 20;

        if (fleet_y & 31) {
          fleet_y++; /* still descending one row */
        } else if (fleet_y & 32) {
          fleet_x++; /* moving right */
          if (fleet_x + edge_col > 300)
            fleet_y++;
        } else {
          fleet_x--; /* moving left */
          if (fleet_x + edge_col < 10)
            fleet_y++;
        }
      }

      if (shot_y > 140) {
        exit(remaining); /* dropout: the piece hit the ground */
      } else if (shot_y > -20) {
        shot_y -= invaders ? 5 : -1; /* missile up, piece down */
        MOVE(SHOT, shot_x, shot_y);
      } else if (!invaders) {
        /* drop a new piece from a random place */
        shot_y = 5;
        srandom(time(0));
        shot_x = random() % 88 + 6;
      }
    } else {
      /* breakout: move the ball, bouncing off the walls and the ceiling */
      if (ball_dx > 0) {
        if (shot_x > 105)
          ball_dx = -ball_dx;
      } else if (shot_x < 1) {
        ball_dx = -ball_dx;
      }
      shot_x += ball_dx;

      if (ball_dy > 0) {
        if (shot_y > 140)
          exit(++remaining); /* missed the ball */
      } else if (shot_y < 1) {
        ball_dy = -ball_dy;
      }
      shot_y += ball_dy;
      MOVE(BALL, shot_x, shot_y);
    }
    XSync(display, 0);

    while (XCheckMaskEvent(display,
                           ButtonPressMask | ButtonReleaseMask |
                               VisibilityChangeMask,
                           &ev)) {
      if (ev.type == ButtonPress) {
        if (ev.xbutton.button & 1 || !invaders) {
          /* button 1 = left, button 3 = right, button 2 = stop */
          player_dx = ev.xbutton.button * 4 - 8;
          check = player_dx == 0;
        } else {
          if (shot_y < 0) { /* fire if no missile is flying */
            shot_x = player_x + 4;
            shot_y = 122;
          }
          check = 0;
        }
      } else if (ev.type == ButtonRelease && ev.xbutton.button) {
        player_dx = 0;
        check = 0;
      } else {
        check = ev.type == VisibilityNotify &&
                ev.xvisibility.state == VisibilityPartiallyObscured;
      }

      if (check) {
        /*
         * Find the window that got covered. In invaders the scan continues
         * from the index left over by the fleet loop (or by the previous
         * scan), so only the first collision of a frame is looked at.
         */
        if (invaders) {
          shot_y = -15; /* the missile is spent */
          n++;
        } else {
          n = 0;
        }
        while (++n < last) {
          if (win[n] == ev.xany.window) {
            if (invaders || n != PLAYER) {
              XUnmapWindow(display, win[n]);
              win[n] = 0;
              remaining--;
            }
            hit = 1;
          }
        }
      }
    }

    if (breakout && hit) {
      ball_dy = -ball_dy; /* bounce off a brick or the paddle */
      hit = 0;
    }
    if (hit && dropout) {
      shot_y = -20; /* piece caught */
      remaining--;
      hit = 0;
    }

    /* keep the paddle inside the playfield; the ship may leave and die */
    if (!invaders && (player_x < 5 && player_dx < 0 ||
                      player_x > 95 && player_dx > 0))
      player_dx = 0;
    if (!win[PLAYER])
      exit(++remaining); /* the ship was destroyed */
    player_x += dropout ? player_dx / 2 : player_dx;
    MOVE(PLAYER, player_x, 130);
  }
  return (remaining);
}
