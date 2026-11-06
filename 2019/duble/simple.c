#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

/*
 * The original Makefile passes these on the command line:
 *   V = terminal lines, W = terminal columns,
 *   g = termios local flags to toggle (ISIG|ICANON|ECHO),
 *   s = first byte of the client socket names (0 = Linux abstract namespace)
 */
#ifndef V
#define V 24
#endif
#ifndef W
#define W 80
#endif
#ifndef g
#define g (ISIG | ICANON | ECHO)
#endif
#ifndef s
#define s 0
#endif

#define ROWS V
#define COLUMNS W

/*
 * The drawing is stored as an executable "#!/bin/cat" script.  Each cell is
 * the 11 byte sequence "\e[A;3F;4Bm C" (attribute, foreground, background,
 * character) and each row ends with "\e[0m\n".
 */
#define SHEBANG     "#!/bin/cat\n\e[1A"
#define BLACK_CELL  "\e[0;30;40m "
#define ROW_END     "\e[0m\n"
#define CLEAR_SCREEN "\e[0m\e[2J\e[0;0H"
#define CURSOR_HOME "\e[0;0H"

#define HEADER_SIZE 15                     /* strlen(SHEBANG) */
#define CELL_SIZE   11                     /* strlen(BLACK_CELL) */
#define ROW_SIZE    (CELL_SIZE * COLUMNS + 5)
enum { DRAWING_SIZE = ROWS * ROW_SIZE + 16 }; /* header + rows + '\0' */

/* offsets of the digits / character inside a cell */
#define CELL_ATTR 2
#define CELL_FG   5
#define CELL_BG   8
#define CELL_CHAR 10

/* well known file descriptors */
#define KEYBOARD_FD   0
#define CLIENT_FD     3   /* every instance talks to the server with this */
#define SERVER_FD     4   /* bound to the drawing file path if we serve */
#define SERVER_OWN_ID 5   /* server's per-client socket for its own client */

/* message types */
enum {
	MSG_REJOIN,        /* server -> client: join the new server */
	MSG_WELCOME,       /* server -> client: here is your id */
	MSG_HANDOVER,      /* server -> client: you are the new server */
	MSG_FORWARD,       /* server -> clients: relay a received message */
	MSG_FILE_BYTE,     /* server -> client: one byte of the drawing */
	MSG_FILE_REQUEST,  /* client -> server: send me byte number offset */
	MSG_JOIN,          /* client -> server: hello */
	MSG_QUIT,          /* client -> server: I am leaving */
	MSG_HANDOVER_ACK,  /* new server -> old server: I took over */
	MSG_CURSOR,        /* client -> server: cursor/stroke update */
	MSG_COMMIT         /* client -> server: draw the line for real */
};

/* keys are reduced modulo 65 so that 'A'..'D' of arrow escapes are 0..3 */
#define KEY(ch)   ((ch) % 65)
#define KEY_UP    KEY('A')
#define KEY_DOWN  KEY('B')
#define KEY_RIGHT KEY('C')
#define KEY_LEFT  KEY('D')
#define KEY_COLOR KEY('c')
#define KEY_QUIT  KEY('q')
#define MODE_MOVE KEY('m')
#define MODE_PEN  KEY('p')
#define MODE_LINE KEY('l')

#define BLINK_USEC (3 << 19)
#define NUM_COLORS 6

struct message {
	int type;
	int client_id;
	int new_id;
	int color;
	int cursor_on;
	int col;
	int row;
	int anchor_col;
	int anchor_row;
	int mode;
	int offset;
	int byte;
} received, mine;

int sock;             /* socket used by send_msg() and receive() */
int max_fd;           /* highest per-client socket of the server */
fd_set readable, watched;
struct sockaddr_un sender_addr, server_addr, my_addr = { .sun_family = AF_UNIX };
socklen_t addr_len = sizeof(server_addr);
char drawing[DRAWING_SIZE];
char *filename;
struct termios term;

/* create a datagram socket, binding it to the file path if it gets fd 4 */
void open_socket(void)
{
	int n;

	for (n = 1; n < 9; n++)
		my_addr.sun_path[n] = rand() % 13 + 'A';
	sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	FD_SET(sock, &watched);
	bind(sock, (struct sockaddr *)(sock == SERVER_FD ? &server_addr : &my_addr), addr_len);
}

void receive(void)
{
	recvfrom(sock, &received, sizeof(received), 0,
		 (struct sockaddr *)&sender_addr, &addr_len);
}

/* load the saved drawing (if any) and try to bind the server socket */
void try_become_server(void)
{
	close(SERVER_FD);
	if (fopen(filename, "r")) {
		/* the FILE got the lowest free descriptor: SERVER_FD */
		read(SERVER_FD, drawing, DRAWING_SIZE);
		close(SERVER_FD);
		unlink(filename);
	}
	open_socket();
	sock = CLIENT_FD;
}

int send_msg(int type)
{
	struct sockaddr_un *dest;

	mine.type = type;
	if (type < MSG_FILE_REQUEST)
		dest = 0;              /* connected socket */
	else if (type == MSG_HANDOVER_ACK)
		dest = &sender_addr;
	else
		dest = &server_addr;
	return sendto(sock, type == MSG_FORWARD ? &received : &mine,
		      sizeof(mine), 0, (struct sockaddr *)dest, dest ? addr_len : 0);
}

/* draw from the cursor to the anchor with Bresenham's line algorithm */
void draw_stroke(void)
{
	int x = received.col, y = received.row;
	int x_end = received.anchor_col, y_end = received.anchor_row;
	int show_star = received.cursor_on;
	int step_x = x < x_end ? 1 : -1, dx = (x_end - x) * step_x;
	int step_y = y < y_end ? 1 : -1, dy = (y_end - y) * step_y;
	int err = (dx > dy ? dx : -dy) / 2, old_err;
	char *cell;

	for (;;) {
		cell = drawing + HEADER_SIZE + (y + 1) * ROW_SIZE + (x + 1) * CELL_SIZE;
		if (received.mode == MODE_PEN ||
		    (received.mode == MODE_LINE && received.type == MSG_COMMIT))
			cell[CELL_BG] = '0' + received.color;
		/* black text if the background already has the pen color */
		cell[CELL_FG] = '0' + (received.color + '0' != cell[CELL_BG] ? received.color : 0);
		/* inverse video shows the foreground color */
		cell[CELL_ATTR] = '0' + received.cursor_on * 7;
		cell[CELL_CHAR] = show_star ? '*' : ' ';
		printf("\e[%d;%dH%.11s\e[%d;%dH\e[0;4%dm%c%s%s",
		       y + 2, x + 2, cell, ROWS, 8, mine.color, mine.mode + 65,
		       CURSOR_HOME, BLACK_CELL);
		if (x == x_end && y == y_end)
			break;
		old_err = err;
		if (old_err > -dx) {
			err -= dy;
			x += step_x;
		}
		if (old_err < dy) {
			err += dx;
			y += step_y;
		}
		show_star = 0;
	}
}

void handle_key(void)
{
	char key;

	sock = CLIENT_FD;
	read(KEYBOARD_FD, &key, 1);
	key %= 65;
	if (key == KEY_COLOR)
		mine.color = mine.color % NUM_COLORS + 1;
	if (key == MODE_PEN || key == MODE_LINE) {
		if (key == mine.mode) {
			mine.cursor_on = 0;
			send_msg(MSG_COMMIT);
			mine.mode = MODE_MOVE;
		} else {
			mine.mode = key;
		}
	}
	if (key < 4) {
		mine.cursor_on = 0;
		send_msg(MSG_CURSOR);
		if (key == KEY_LEFT && mine.col)
			mine.col--;
		if (key == KEY_RIGHT && mine.col != COLUMNS - 3)
			mine.col++;
		if (key == KEY_UP && mine.row)
			mine.row--;
		if (key == KEY_DOWN && mine.row != ROWS - 3)
			mine.row++;
		mine.cursor_on = 1;
	}
	if (mine.mode != MODE_LINE) {
		mine.anchor_col = mine.col;
		mine.anchor_row = mine.row;
	}
	send_msg(MSG_CURSOR);
	if (key == KEY_QUIT) {
		mine.cursor_on = 0;
		send_msg(MSG_CURSOR);
		send_msg(MSG_QUIT);
	}
}

void serve(void)
{
	sock = SERVER_FD;
	receive();
	if (received.type == MSG_JOIN) {
		open_socket();
		mine.new_id = sock;
		connect(sock, (struct sockaddr *)&sender_addr, addr_len);
		if (sock > max_fd)
			max_fd = sock;
		send_msg(MSG_WELCOME);
	} else if (received.type == MSG_FILE_REQUEST) {
		mine.byte = drawing[received.offset];
		sock = received.client_id;
		send_msg(MSG_FILE_BYTE);
	} else {
		for (sock = SERVER_OWN_ID; sock <= max_fd; sock++)
			send_msg(MSG_FORWARD);
	}
}

/* save the drawing and pass the server role to the next client */
void hand_over(void)
{
	unlink(filename);
	write(creat(filename, 0777), drawing, DRAWING_SIZE);
	for (sock = SERVER_OWN_ID + 1; sock <= max_fd; sock++)
		if (send_msg(MSG_HANDOVER) != -1) {
			receive();  /* wait for MSG_HANDOVER_ACK */
			for (sock++; sock <= max_fd; sock++)
				send_msg(MSG_REJOIN);
		}
}

/* returns 1 when this instance should exit */
int handle_message(void)
{
	sock = CLIENT_FD;
	receive();
	switch (received.type) {
	case MSG_HANDOVER:
		try_become_server();
		send_msg(MSG_JOIN);
		send_msg(MSG_HANDOVER_ACK);
		return 0;
	case MSG_REJOIN:
		send_msg(MSG_JOIN);
		break;
	case MSG_WELCOME:
		mine.client_id = received.new_id;
		if (!mine.color)
			mine.color = mine.client_id % NUM_COLORS + 1;
		if (!mine.offset)
			send_msg(MSG_FILE_REQUEST);
		break;
	case MSG_FILE_BYTE:
		drawing[mine.offset++] = received.byte;
		if (mine.offset < DRAWING_SIZE)
			send_msg(MSG_FILE_REQUEST);
		else
			printf("%s%.*s\r \e[97mmode:", CLEAR_SCREEN, DRAWING_SIZE - 6, drawing);
		break;
	case MSG_QUIT:
		if (mine.client_id == SERVER_OWN_ID)
			close(received.client_id);
		if (received.client_id == mine.client_id) {
			if (mine.client_id == SERVER_OWN_ID)
				hand_over();
			printf(CLEAR_SCREEN);
			return 1;
		}
		break;
	}
	if (received.type > MSG_HANDOVER_ACK)
		draw_stroke();
	fflush(stdout);
	return 0;
}

void toggle_raw_mode(void)
{
	tcgetattr(1, &term);
	term.c_lflag ^= g;
	tcsetattr(1, TCSANOW, &term);
}

int main(int argc, char **argv)
{
	int row, col, usec, blink_deadline = 0;
	struct timeval now, timeout = { 0, 0 };

	strcat(drawing, SHEBANG);
	for (row = 0; row < ROWS; row++) {
		for (col = 0; col < COLUMNS; col++)
			strcat(drawing, BLACK_CELL);
		strcat(drawing, ROW_END);
	}

	if (argc != 2)
		return 0;

	filename = argv[1];
	FD_SET(KEYBOARD_FD, &watched);
	srand(getpid());
	strcpy(my_addr.sun_path, filename);
	server_addr = my_addr;
	my_addr.sun_path[0] = s;
	mine.mode = MODE_MOVE;
	open_socket();          /* CLIENT_FD */
	try_become_server();    /* SERVER_FD */
	send_msg(MSG_JOIN);

	toggle_raw_mode();
	for (;;) {
		gettimeofday(&now, 0);
		usec = now.tv_usec;
		if (!blink_deadline)
			blink_deadline = BLINK_USEC + usec;
		timeout.tv_usec = (blink_deadline - usec) % 999999;
		sock = CLIENT_FD;
		readable = watched;
		if (!select(SERVER_FD + 1, &readable, 0, 0, &timeout)) {
			mine.cursor_on ^= 1;
			send_msg(MSG_CURSOR);
			blink_deadline = 0;
		}
		if (FD_ISSET(KEYBOARD_FD, &readable))
			handle_key();
		if (FD_ISSET(SERVER_FD, &readable))
			serve();
		if (FD_ISSET(CLIENT_FD, &readable) && handle_message())
			break;
	}
	toggle_raw_mode();
	return 0;
}
