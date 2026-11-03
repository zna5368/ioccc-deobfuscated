#include <unistd.h>
#include <curses.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>

#ifndef PORT
#define PORT 5455
#endif
#ifndef SPEED
#define SPEED 50 /* milliseconds per frame */
#endif

/* single byte messages sent to the opponent once per frame */
#define MSG_LEFT '['  /* my paddle moved left */
#define MSG_RIGHT 0   /* my paddle moved right */
#define MSG_STILL ']' /* my paddle did not move */
#define MSG_QUIT '='  /* the server quit the game */

#define BUFSIZE (1 << 9)

/* text of a paddle, "[=====...=]" */
char paddle[BUFSIZE];

/* draw a paddle at column x of the given row, padded with a space on both
   sides to erase the previous position */
void draw_paddle(long x, long row) {
  mvprintw(row, x ? x - 1 : x, "%s%s ", x ? " " : "", paddle);
}

void send_byte(int sock, char byte) { write(sock, &byte, 1); }

/* microseconds elapsed between two times */
long elapsed_usec(struct timeval *start, struct timeval *end) {
  long sec = end->tv_sec - start->tv_sec;
  long usec = end->tv_usec - start->tv_usec;
  if (usec < 0) {
    usec += 1000000;
    sec--;
  }
  return sec * 1000000 + usec;
}

int main(int argc, char *argv[]) {
  struct sockaddr_in addr;
  struct hostent *host;
  struct timeval frame_start, frame_end, no_wait;
  socklen_t addr_len = sizeof(addr);
  fd_set readable;
  char message;
  int sock, is_server, key, i;
  long paddle_width, rows, bottom_row, middle_row;
  long my_x, their_x, my_move, their_move;
  long my_score = 0, their_score = 0;
  double ball_x, ball_dx = 0.2, old_ball_x = 0;
  long ball_y, ball_dy = 1, old_ball_y = 0;

  initscr();
  cbreak();
  noecho();
  nonl();

  /* build the paddle string, a quarter of the screen wide */
  paddle_width = COLS / 4;
  for (i = 0; i < paddle_width; i++)
    paddle[i] = '=';
  paddle[paddle_width] = '\0';
  paddle[0] = '[';
  paddle[paddle_width - 1] = ']';

  /* connect the two players */
  for (i = 0; i < (int)sizeof(addr); i++)
    ((char *)&addr)[i] = 0;
  addr.sin_port = htons((unsigned long)(PORT & 0xffff));
  sock = socket(addr.sin_family = AF_INET, SOCK_STREAM, 0);
  is_server = (argc == 1);
  if (is_server) {
    addr.sin_addr.s_addr = 0;
    bind(sock, (struct sockaddr *)&addr, addr_len);
    listen(sock, 5);
    sock = accept(sock, (struct sockaddr *)&addr, &addr_len);
  } else {
    ball_dy = -1; /* the client sees the field upside down */
    host = gethostbyname(argv[1]);
    for (i = 0; i < host->h_length; i++)
      ((char *)&addr.sin_addr)[i] = host->h_addr_list[0][i];
    addr.sin_family = host->h_addrtype;
    connect(sock, (struct sockaddr *)&addr, addr_len);
  }

  /* an odd number of rows so there is an exact middle line */
  rows = (LINES % 2) ? LINES : LINES - 1;
  my_x = their_x = (ball_x = COLS * 0.5) - paddle_width / 2;
  bottom_row = rows - 1;
  ball_y = middle_row = rows * 0.5;

  for (;;) {
    /* draw the field */
    mvaddch(old_ball_y, old_ball_x, ' ');
    draw_paddle(my_x, bottom_row);
    draw_paddle(their_x, 0);
    mvprintw(middle_row - 1, 0, "%d\n\n%d", their_score, my_score);
    mvhline(middle_row, 0, '-', COLS);
    mvaddch(ball_y, ball_x, '*');
    move(middle_row, 0);
    refresh();

    /* wait up to one frame for a key and discard any extra keys */
    timeout(SPEED);
    gettimeofday(&frame_start, 0);
    key = getch();
    timeout(0);
    while (getch() != ERR)
      ;

    /* tell the opponent what happened */
    if (key == 'q' && is_server)
      send_byte(sock, MSG_QUIT);
    my_move = 0;
    if (my_x > 0 && key == ',') {
      my_move = -1;
      send_byte(sock, MSG_LEFT);
    } else if (key == '.' && my_x + paddle_width < COLS) {
      my_move = 1;
      send_byte(sock, MSG_RIGHT);
    } else {
      send_byte(sock, MSG_STILL);
    }

    /* sleep out the rest of the frame if a key arrived early */
    gettimeofday(&frame_end, 0);
    long remaining = elapsed_usec(&frame_start, &frame_end) - SPEED * 1000;
    usleep(remaining < 0 ? -remaining : 0);

    if (key == 'q' && !is_server)
      break;

    /* hear what the opponent did */
    their_move = 0;
    FD_ZERO(&readable);
    FD_SET(sock, &readable);
    no_wait.tv_sec = no_wait.tv_usec = 0;
    select(sock + 1, &readable, 0, 0, &no_wait);
    if (read(sock, &message, 1)) {
      if (message == MSG_RIGHT)
        their_move = 1;
      else if (message == MSG_LEFT)
        their_move = -1;
      else if (message == MSG_QUIT)
        break;
    } else {
      break; /* the opponent disconnected */
    }

    /* move the ball */
    old_ball_x = ball_x;
    ball_x += ball_dx;
    old_ball_y = ball_y;
    ball_y += ball_dy;

    /* bounce off the side walls */
    if (ball_x < 0) {
      ball_x = -ball_x;
      ball_dx = -ball_dx;
    }
    if (ball_x > COLS) {
      ball_x = COLS + COLS - ball_x;
      ball_dx = -ball_dx;
    }

    /* the opponent's paddle at the top, its motion puts spin on the ball */
    their_x += their_move;
    if (ball_x >= their_x && ball_y < 1 && ball_dy < 0 &&
        ball_x < their_x + paddle_width) {
      ball_y = 2;
      ball_dy = -ball_dy;
      ball_dx += their_move * 0.4;
    }
    /* the opponent missed, the ball comes back in from the bottom */
    if (ball_y < 0) {
      ball_y = bottom_row;
      my_score++;
    }

    /* my paddle at the bottom */
    my_x += my_move;
    if (ball_x >= my_x && ball_y > bottom_row - 1 && ball_dy > 0 &&
        ball_x < my_x + paddle_width) {
      ball_y = bottom_row - 2;
      ball_dy = -ball_dy;
      ball_dx += my_move * 0.4;
    }
    /* I missed, the ball comes back in from the top */
    if (ball_y > bottom_row) {
      ball_y = 0;
      their_score++;
    }
  }
  endwin();
  return (0);
}
