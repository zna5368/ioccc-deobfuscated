#include <stdio.h>

/*
 * Compile time settings. The defaults are the ones used to build prog, which
 * is a copy of gray-scott-1. Writing -Delta_u=... on the compiler command line
 * means "-D elta_u=...", which is why these two macros are missing the "D".
 */
#ifndef elta_u
#define elta_u Du*laplacian(u)-u*v*v+F*(1-u)
#endif
#ifndef elta_v
#define elta_v Dv*laplacian(v)+u*v*v-v*(F+K)
#endif
#ifndef Du
#define Du 0.100
#endif
#ifndef Dv
#define Dv 0.050
#endif
#ifndef F
#define F 0.035
#endif
#ifndef K
#define K 0.060
#endif

/* These hold "u,v" so that they can be used as a comma expression. */
#ifndef UV_BACKGROUND
#define UV_BACKGROUND 1,0
#endif
#ifndef UV_DROP
#define UV_DROP -0.5,0.5
#endif

#ifndef TIMESTEP
#define TIMESTEP 1.0
#endif
#ifndef SPEED
#define SPEED 8
#endif

/* This holds "red:green:blue" so that it can finish a conditional expression. */
#ifndef RGB
#define RGB 255:128:192
#endif

/* Lets the model expressions above say laplacian(u) and laplacian(v). */
#define laplacian(substance) laplacian_##substance

#define GRID_SIZE 128
#define SUBSTANCE_U 0
#define SUBSTANCE_V 1
#define DROP_RADIUS_SQUARED 20

#define LINE_BUFFER_SIZE 9999
#define MAX_LINE_LENGTH 480
#define KEY_HEADER_NAME "sec-websocket-key:"
#define KEY_LENGTH 24
#define WEBSOCKET_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define HASHED_MESSAGE_LENGTH 60
#define ACCEPT_KEY_LENGTH 27

#define BYTES_PER_PIXEL 4
#define MASK_LENGTH 4
#define MAX_PAYLOAD_LENGTH 127

/*
 * The text after "Connection:" in the reply to a plain web request. The spaces
 * in the script come from the shape of the original source. They are kept so
 * that the page is sent byte for byte the same.
 */
const char *html_reply =
    "close\r\n"
    "Content-Type:text/html\r\n"
    "\r\n"
    "<canvas id='c'width='128'height='128'"
    "style='width:256px;height:256px'>"
    "<script>"
    "x=y=-1;"
    "v=window.c;"
    "v.onmousemove=function(e){"
    "x=e.pageX-v.offsetLeft;"
    "y=e.               pageY-v.offsetTop"
    "};"
    "w=new WebSocket('ws'+                     location.href.substr(4));"
    "w.binaryType='arraybuffer';"
    "w.onmessage=function(e){"
    "c=v.getContext('2d');"
    "b=c.createImageData(128,128);"
    "u=        new Uint8Array(d=e.           data);"
    "b.data.set(u)      ;"
    "c.putImageData(b,0,0)            ;"
    "u[0]=x/2;"
    "u[1]=y/2    ;"
    "w.send(d.slice(0,x<0?0:             2));"
    "x=y=-1"
    "}"
    "</script>";

const char *websocket_reply =
    "upgrade\r\n"
    "Upgrade:websocket\r\n"
    "Sec-WebSocket-Accept:";

/*
 * grid[y][x][buffer][substance]. Every cell keeps two copies of u and v. Each
 * simulation step reads one copy and writes the other, then they swap roles.
 * Copy 0 is the one that is drawn and that drops are added to.
 */
float grid[GRID_SIZE][GRID_SIZE][2][2];

/* Not cleared between lines: a short key runs on into the previous lines. */
char line[LINE_BUFFER_SIZE];

/* The first SHA-1 block: the key, the GUID and the padding bit. */
unsigned int message_block[16];

unsigned int sha1_state[5] = {
    0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0
};

unsigned int rotate_left(unsigned int value, int count)
{
    return value << count | value >> (32 - count);
}

/*
 * The original worked out the round constants at run time, starting from the
 * last header byte it looked at. If that byte was zero, or there were no header
 * lines at all, it divided by zero and crashed. Fixed constants cannot do that.
 */
void sha1_process_block(const unsigned int *block)
{
    unsigned int schedule[80];
    unsigned int a = sha1_state[0];
    unsigned int b = sha1_state[1];
    unsigned int c = sha1_state[2];
    unsigned int d = sha1_state[3];
    unsigned int e = sha1_state[4];
    unsigned int mix, round_constant, next;
    int round;

    for (round = 0; round < 16; round++)
        schedule[round] = block[round];
    for (round = 16; round < 80; round++)
        schedule[round] = rotate_left(schedule[round - 3] ^ schedule[round - 8] ^
                                      schedule[round - 14] ^ schedule[round - 16], 1);

    for (round = 0; round < 80; round++) {
        if (round < 20) {
            mix = (b & c) | (~b & d);
            round_constant = 0x5A827999;
        } else if (round < 40) {
            mix = b ^ c ^ d;
            round_constant = 0x6ED9EBA1;
        } else if (round < 60) {
            mix = (b & c) | ((b | c) & d);
            round_constant = 0x8F1BBCDC;
        } else {
            mix = b ^ c ^ d;
            round_constant = 0xCA62C1D6;
        }
        next = rotate_left(a, 5) + e + round_constant + mix + schedule[round];
        e = d;
        d = c;
        c = rotate_left(b, 30);
        b = a;
        a = next;
    }

    sha1_state[0] += a;
    sha1_state[1] += b;
    sha1_state[2] += c;
    sha1_state[3] += d;
    sha1_state[4] += e;
}

/* Runs SHA-1 over the 60 byte message in message_block. */
void sha1_hash_message(void)
{
    unsigned int length_block[16] = { 0 };

    /* The message plus padding does not leave room for its length. */
    length_block[15] = HASHED_MESSAGE_LENGTH * 8;

    sha1_process_block(message_block);
    sha1_process_block(length_block);
}

/* Writes the 160 bit hash as 27 Base64 digits, leaving out the final "=". */
void base64_encode_hash(char *digits)
{
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int digit, bit;

    for (digit = 0; digit < ACCEPT_KEY_LENGTH; digit++) {
        int value = 0;

        for (bit = digit * 6; bit < digit * 6 + 6; bit++) {
            value *= 2;
            if (bit < 160)
                value += sha1_state[bit / 32] >> (31 - bit % 32) & 1;
        }
        digits[digit] = alphabet[value];
    }
}

/* Header names are matched case insensitively by forcing them to lower case. */
int is_websocket_key_header(void)
{
    static const char name[] = KEY_HEADER_NAME;
    int position;

    for (position = 0; name[position]; position++)
        if (name[position] != (line[position] | 32))
            return 0;
    return 1;
}

void add_message_byte(int index, char byte)
{
    message_block[index / 4] |= (unsigned int)byte << (3 - index % 4) * 8;
}

/* Builds the SHA-1 message: key + GUID + the 0x80 byte that starts the padding. */
void read_websocket_key(void)
{
    static const char guid[] = WEBSOCKET_GUID;
    const char *text = line + sizeof KEY_HEADER_NAME - 1;
    int index = 0;

    for (; index < KEY_LENGTH; text++)
        if (*text != ' ')
            add_message_byte(index++, *text);
    for (; index < HASHED_MESSAGE_LENGTH; index++)
        add_message_byte(index, guid[index - KEY_LENGTH]);
    add_message_byte(index, '\x80');
}

void read_request_headers(void)
{
    while (fgets(line, MAX_LINE_LENGTH, stdin) && line[0] > 31)
        if (is_websocket_key_header())
            read_websocket_key();
}

/*
 * The accept key is always sent, even after the HTML page where it means
 * nothing, because both replies share a single format string.
 */
void send_http_reply(int is_websocket)
{
    char accept_key[ACCEPT_KEY_LENGTH];

    sha1_hash_message();
    base64_encode_hash(accept_key);
    printf("HTTP/1.1 %d OK\r\nConnection:%s%.27s=\r\n\r\n",
           is_websocket ? 101 : 200,
           is_websocket ? websocket_reply : html_reply,
           accept_key);
}

/* Only copy 0 is filled, so the border of copy 1 stays zero for ever. */
void fill_grid_with_background(void)
{
    int x, y;

    for (y = 0; y < GRID_SIZE; y++) {
        for (x = 0; x < GRID_SIZE; x++) {
            float *cell = grid[y][x][0];

            cell[SUBSTANCE_V] = (cell[SUBSTANCE_U] = UV_BACKGROUND);
        }
    }
}

void add_drop(float center_x, float center_y)
{
    int x, y;

    for (y = 1; y < GRID_SIZE - 1; y++) {
        for (x = 1; x < GRID_SIZE - 1; x++) {
            float distance_x = x - center_x;
            float distance_y = y - center_y;

            if (distance_x * distance_x + distance_y * distance_y < DROP_RADIUS_SQUARED) {
                float *cell = grid[y][x][0];

                cell[SUBSTANCE_V] += (cell[SUBSTANCE_U] += UV_DROP);
            }
        }
    }
}

/* The pattern file is a list of "x,y," pairs. Each pair adds one drop. */
void add_drops_from_file(const char *file_name)
{
    FILE *pattern = fopen(file_name, "r");
    float x = 0, y = 0;

    if (!pattern)
        return;
    while (fscanf(pattern, "%f,%f,", &x, &y) > 0)
        add_drop(x, y);
    fclose(pattern);
}

/* The sum of the four neighbours minus four times the cell itself. */
float grid_laplacian(int x, int y, int buffer, int substance)
{
    return grid[y - 1][x][buffer][substance] + grid[y][x - 1][buffer][substance] +
           grid[y][x + 1][buffer][substance] + grid[y + 1][x][buffer][substance] -
           grid[y][x][buffer][substance] * 4;
}

/* The outermost ring of cells is never updated. */
void simulate_step(int source, int destination)
{
    float timestep = TIMESTEP;
    int x, y;

    for (y = 1; y < GRID_SIZE - 1; y++) {
        for (x = 1; x < GRID_SIZE - 1; x++) {
            float u = grid[y][x][source][SUBSTANCE_U];
            float v = grid[y][x][source][SUBSTANCE_V];
            float laplacian_u = grid_laplacian(x, y, source, SUBSTANCE_U);
            float laplacian_v = grid_laplacian(x, y, source, SUBSTANCE_V);

            grid[y][x][destination][SUBSTANCE_U] = u + timestep * (elta_u);
            grid[y][x][destination][SUBSTANCE_V] = v + timestep * (elta_v);
        }
    }
}

/* Each pass is two steps, so the newest values always end up back in copy 0. */
void simulate_frame(void)
{
    int pass;

    for (pass = 0; pass < SPEED; pass++) {
        simulate_step(0, 1);
        simulate_step(1, 0);
    }
}

int color_of_channel(int channel)
{
    return channel < 2 ? channel == 0 ? RGB;
}

/* The brightness of a pixel comes from the amount of v in its cell. */
int pixel_channel(float v, int channel)
{
    float value = v * 4 * color_of_channel(channel);

    if (value > 255)
        return 255;
    if (value < 0)
        return 0;
    return value;
}

/* A WebSocket binary frame holding one RGBA canvas image. */
void send_frame(void)
{
    unsigned long long length = GRID_SIZE * GRID_SIZE * BYTES_PER_PIXEL;
    int shift, x, y, channel;

    putchar(0x82);  /* last part of the message, binary data */
    putchar(0x7F);  /* the length follows as a 64 bit number */
    for (shift = 56; shift >= 0; shift -= 8)
        putchar(length >> shift & 255);

    for (y = 0; y < GRID_SIZE; y++) {
        for (x = 0; x < GRID_SIZE; x++) {
            for (channel = 0; channel < 3; channel++)
                putchar(pixel_channel(grid[y][x][0][SUBSTANCE_V], channel));
            putchar(255);  /* alpha */
        }
    }
    fflush(stdout);
}

/*
 * The browser answers every frame with a masked frame. Its payload is empty,
 * or else it holds the mouse position, which is then used as a new drop. Only
 * the short form of the length is understood, and only 7 bits of each byte are
 * kept, which is enough for coordinates below 128.
 */
void receive_mouse_position(void)
{
    char bytes[MASK_LENGTH + MAX_PAYLOAD_LENGTH];
    int payload_length, index;

    getchar();  /* frame type */
    payload_length = getchar() & 127;
    for (index = 0; index < MASK_LENGTH + payload_length; index++)
        bytes[index] = getchar() & 127;

    /* bytes[] is the mask then the payload, so a mask byte is 4 bytes before. */
    if (payload_length > 1) {
        int x_index = payload_length - 2;
        int y_index = payload_length - 1;

        add_drop(bytes[x_index] ^ bytes[x_index + MASK_LENGTH],
                 bytes[y_index] ^ bytes[y_index + MASK_LENGTH]);
    }
}

int main(int argc, char **argv)
{
    int is_websocket;

    read_request_headers();
    is_websocket = message_block[0] != 0;
    send_http_reply(is_websocket);
    fill_grid_with_background();

    if (is_websocket) {
        if (argc >= 2)
            add_drops_from_file(argv[1]);
        for (;;) {
            simulate_frame();
            send_frame();
            receive_mouse_position();
        }
    }
    return 0;
}
