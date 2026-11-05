#include <math.h>
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>

#define SERVER_PORT 8224
#define LISTEN_BACKLOG 10

#define IMAGE_WIDTH 800
#define IMAGE_HEIGHT 600

#define MAX_BOUNCES 8
#define MAX_SPHERES 333
#define LIGHT_COUNT 3
#define LIGHT_RADIUS_SQUARED 99
#define DOT_RADIUS_SQUARED .6
#define FLOOR_HEIGHT (-1)

/* Values returned by cast_ray() when the ray did not hit a sphere. */
#define HIT_NOTHING (-1)
#define HIT_FLOOR (-2)

#define GLYPH_COLUMNS 5
#define GLYPH_ROWS 9
#define COLON_GLYPH 10

/* One IDAT chunk holding a single stored deflate block:
 * 4 byte length, 4 byte type, 1 byte block header, 4 bytes LEN and NLEN. */
#define BLOCK_LENGTH_OFFSET 9
#define BLOCK_DATA_OFFSET 13
#define FULL_ROW_BYTES (1 + 3 * IMAGE_WIDTH)
#define CRC_BYTES 4
#define BUFFER_SIZE (BLOCK_DATA_OFFSET + FULL_ROW_BYTES + CRC_BYTES)

#define CRC_POLYNOMIAL 0xEDB88320u
#define ADLER_MODULUS 65521

typedef float Vector[3];

/* Bit map of each digit and the colon, top row first. */
const char *const font[][GLYPH_ROWS] = {
    { " ### ", "#   #", "#   #", "#   #", "#   #", "#   #", "#   #", "#   #", " ### " },
    { "  #  ", " ##  ", "# #  ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "#####" },
    { " ### ", "#   #", "    #", "    #", "  ## ", " #   ", "#    ", "#    ", "#####" },
    { " ### ", "#   #", "    #", "    #", " ### ", "    #", "    #", "#   #", " ### " },
    { "  ## ", " # # ", "#  # ", "#  # ", "#####", "   # ", "   # ", "   # ", "   # " },
    { "#####", "#    ", "#    ", "# ## ", "##  #", "    #", "    #", "#   #", " ### " },
    { "  ## ", " #  #", "#    ", "#    ", "# ## ", "##  #", "#   #", "#   #", " ### " },
    { "#####", "    #", "    #", "   # ", "   # ", "  #  ", "  #  ", " #   ", " #   " },
    { " ### ", "#   #", "#   #", "#   #", " ### ", "#   #", "#   #", "#   #", " ### " },
    { " ### ", "#   #", "#   #", "#  ##", " ## #", "    #", "    #", "   # ", " ##  " },
    { "     ", "     ", "  #  ", "     ", "     ", "     ", "  #  ", "     ", "     " },
};

/* The seven interlacing passes of the PNG Adam7 scheme. */
const struct {
    int x_start, y_start, x_step, y_step;
} adam7_passes[] = {
    { 0, 0, 8, 8 },
    { 4, 0, 8, 8 },
    { 0, 4, 4, 8 },
    { 2, 0, 4, 4 },
    { 0, 2, 2, 4 },
    { 1, 0, 2, 2 },
    { 0, 1, 1, 2 },
};

/* The lights come first, the dots that draw the time are added after them. */
Vector spheres[MAX_SPHERES] = {
    { 0, -9, 70 },
    { 130, 50, 30 },
    { -60, 40, 30 },
};
int sphere_count;

Vector eye = { 0, -22, 37 };

/* Steps of one pixel right and one pixel down on the view plane. */
Vector pixel_right, pixel_down;

/* The ray being traced and what it found. */
Vector ray_origin, ray_direction, colour;
float hit_distance;

unsigned char buffer[BUFFER_SIZE];
unsigned char *write_position = buffer;
int client_socket;

/* Running Adler-32 checksum of the uncompressed image data. */
int adler_low, adler_high;


void vector_copy(float *destination, const float *source)
{
    destination[0] = source[0];
    destination[1] = source[1];
    destination[2] = source[2];
}

void vector_add(float *destination, const float *source)
{
    destination[0] += source[0];
    destination[1] += source[1];
    destination[2] += source[2];
}

void vector_subtract(float *destination, const float *source)
{
    destination[0] -= source[0];
    destination[1] -= source[1];
    destination[2] -= source[2];
}

float vector_dot(const float *first, const float *second)
{
    float sum = first[0] * second[0];
    sum += first[1] * second[1];
    return sum += first[2] * second[2];
}

void vector_scale(float *vector, float factor)
{
    vector[0] *= factor;
    vector[1] *= factor;
    vector[2] *= factor;
}

void vector_add_to_all(float *vector, float amount)
{
    vector[0] += amount;
    vector[1] += amount;
    vector[2] += amount;
}

float vector_length(const float *vector)
{
    return sqrt(vector_dot(vector, vector));
}

void vector_normalize(float *vector)
{
    vector_scale(vector, 1 / vector_length(vector));
}


void put_byte(int value)
{
    *write_position++ = value;
}

void put_uint32(uint32_t value)
{
    put_byte(value >> 24);
    put_byte(value >> 16);
    put_byte(value >> 8);
    put_byte(value);
}

void put_string(const char *text)
{
    while (*text)
        put_byte(*text++);
}

void flush_buffer(void)
{
    write(client_socket, buffer, write_position - buffer);
    write_position = buffer;
}

/* Send whatever is pending, then start a PNG chunk. The length is filled in by
 * end_chunk(). */
void begin_chunk(const char *type)
{
    flush_buffer();
    write_position += 4;
    put_string(type);
}

/* Fill in the length of the chunk in the buffer, append its CRC and send it. */
void end_chunk(void)
{
    unsigned char *chunk_end = write_position;
    uint32_t crc = ~0u;
    int bit;

    write_position = buffer;
    put_uint32(chunk_end - write_position - 8);

    while (write_position != chunk_end) {
        crc ^= *write_position++;
        for (bit = 0; bit < 8; bit++)
            crc = (crc >> 1) ^ (crc & 1 ? CRC_POLYNOMIAL : 0);
    }

    put_uint32(~crc);
    flush_buffer();
}

void put_image_byte(int value)
{
    put_byte(value);
    adler_low = (adler_low + value) % ADLER_MODULUS;
    adler_high = (adler_high + adler_low) % ADLER_MODULUS;
}

void put_colour_component(float brightness)
{
    brightness = .5 + 255 * brightness;
    put_image_byte(brightness < 0 ? 0 : brightness > 255 ? 255 : brightness);
}


/* Add one digit or colon to the scene as a grid of small spheres lying on the
 * z = 0 plane. Columns are half a unit apart and rows go from bottom to top. */
void add_glyph(float x, int glyph)
{
    int column, row;

    for (column = 0; column < GLYPH_COLUMNS; column++) {
        for (row = GLYPH_ROWS - 1; row >= 0; row--) {
            if (font[glyph][row][column] != ' ') {
                float *sphere = spheres[sphere_count++];

                sphere[0] = x;
                sphere[1] = (GLYPH_ROWS / 2 - row) / 2.0f;
                sphere[2] = 0;
            }
        }
        x += .5;
    }
}

/* Rebuild the scene to show the time as H:MM:SS or HH:MM:SS on a 12 hour clock. */
void build_clock(const struct tm *now)
{
    int hour = 1 + (11 + now->tm_hour) % 12;

    sphere_count = LIGHT_COUNT;

    if (hour > 9)
        add_glyph(-14, hour / 10);
    add_glyph(-10, hour % 10);
    add_glyph(-6, COLON_GLYPH);
    add_glyph(-2, now->tm_min / 10);
    add_glyph(2, now->tm_min % 10);
    add_glyph(6, COLON_GLYPH);
    add_glyph(10, now->tm_sec / 10);
    add_glyph(14, now->tm_sec % 10);
}


/* Follow the ray that starts at ray_origin and passes through the point held in
 * ray_direction, which is replaced by the unit length direction of the ray.
 * Returns the index of the nearest sphere it hits, or HIT_FLOOR or HIT_NOTHING,
 * and sets hit_distance. */
int cast_ray(void)
{
    int nearest = HIT_NOTHING;
    int sphere;
    float floor_hit;

    hit_distance = 1e9;

    vector_subtract(ray_direction, ray_origin);
    vector_normalize(ray_direction);

    if (ray_direction[2]) {
        floor_hit = (ray_origin[2] - FLOOR_HEIGHT) / ray_direction[2];
        if (floor_hit < 0) {
            hit_distance = -floor_hit;
            nearest = HIT_FLOOR;
        }
    }

    for (sphere = 0; sphere < sphere_count; sphere++) {
        Vector to_centre;
        float along_ray, discriminant, distance;

        vector_copy(to_centre, spheres[sphere]);
        vector_subtract(to_centre, ray_origin);
        along_ray = vector_dot(ray_direction, to_centre);

        discriminant = along_ray * along_ray - vector_dot(to_centre, to_centre)
            + (sphere < LIGHT_COUNT ? LIGHT_RADIUS_SQUARED : DOT_RADIUS_SQUARED);

        if (discriminant >= 0) {
            /* An exact distance would take the square root of the discriminant. */
            distance = along_ray < discriminant
                ? discriminant + along_ray
                : along_ray - discriminant;

            if (distance > 0 && hit_distance > distance) {
                hit_distance = distance;
                nearest = sphere;
            }
        }
    }

    return nearest;
}

/* Find the colour seen by the ray and leave it in colour. */
void trace_ray(int depth)
{
    Vector hit_point, normal, offset;
    int hit = cast_ray();
    int on_floor = hit == HIT_FLOOR;
    int light;

    if (hit == HIT_NOTHING) {
        /* Sky: blue that gets stronger towards the zenith. */
        colour[2] = ray_direction[2] / 2;
        colour[0] = colour[1] = colour[2] / 2;
        return;
    }

    if (hit >= 0 && hit < LIGHT_COUNT) {
        colour[0] = colour[1] = colour[2] = 1;
        return;
    }

    vector_copy(hit_point, ray_direction);
    vector_scale(hit_point, hit_distance);
    vector_add(hit_point, ray_origin);

    if (on_floor) {
        normal[0] = normal[1] = 0;
        normal[2] = 1;
    } else {
        vector_copy(normal, hit_point);
        vector_subtract(normal, spheres[hit]);
        vector_normalize(normal);
    }

    /* Step off the surface so that new rays do not hit it straight away. */
    vector_copy(offset, normal);
    vector_scale(offset, 1e-4);
    vector_add(hit_point, offset);

    colour[0] = colour[1] = colour[2] = 0;

    if (depth < MAX_BOUNCES) {
        /* Aim a new ray in the mirror direction. */
        vector_copy(offset, normal);
        vector_scale(offset, 2 * vector_dot(normal, ray_direction));
        vector_subtract(ray_direction, offset);
        vector_add(ray_direction, hit_point);
        vector_copy(ray_origin, hit_point);

        trace_ray(depth + 1);
        vector_scale(colour, on_floor ? .2 : .8);
    }

    /* Diffuse lighting from every light that is not in shadow. */
    for (light = 0; light < LIGHT_COUNT; light++) {
        vector_copy(ray_direction, spheres[light]);
        vector_copy(ray_origin, hit_point);

        if (cast_ray() == light)
            vector_add_to_all(colour, (on_floor ? .5 : .1) * vector_dot(normal, ray_direction));
    }

    vector_add_to_all(colour, 0.05);

    /* Checkerboard: dim the blue, then dim red or green depending on the square. */
    if (on_floor) {
        colour[2] *= .3;
        colour[(lrint(hit_point[0]) ^ lrint(hit_point[1])) & 1] *= .2;
    }
}


/* Move the eye from side to side as the seconds go by, then work out the two
 * pixel step vectors. The view plane goes through the origin, which is what the
 * eye is looking at. */
void aim_camera(int second)
{
    float pixel_size;

    eye[0] = second > 45 ? second - 60 : second < 15 ? second : 30 - second;

    pixel_right[0] = -eye[1];
    pixel_right[1] = eye[0];
    pixel_right[2] = 0;

    pixel_down[0] = eye[2] * eye[0];
    pixel_down[1] = eye[2] * eye[1];
    pixel_down[2] = -eye[0] * eye[0] - eye[1] * eye[1];

    vector_normalize(pixel_right);
    vector_normalize(pixel_down);

    pixel_size = vector_length(eye) / 1e3;
    vector_scale(pixel_right, pixel_size);
    vector_scale(pixel_down, pixel_size);
}

/* Start an IDAT chunk holding one row of a pass as a stored deflate block. */
void begin_row(void)
{
    begin_chunk("IDAT");
    put_byte(0);                /* deflate block: stored, not the last one */
    write_position += 4;        /* LEN and NLEN are filled in by end_row() */
    put_image_byte(0);          /* PNG row filter: none */
}

void end_row(void)
{
    int block_length = write_position - buffer - BLOCK_DATA_OFFSET;

    buffer[BLOCK_LENGTH_OFFSET] = block_length;
    buffer[BLOCK_LENGTH_OFFSET + 1] = block_length >> 8;
    buffer[BLOCK_LENGTH_OFFSET + 2] = ~block_length;
    buffer[BLOCK_LENGTH_OFFSET + 3] = ~(block_length >> 8);
    end_chunk();
}

/* Read the HTTP request up to the blank line that ends its headers. */
void skip_request(void)
{
    int line_length = 0;

    for (;;) {
        ++line_length;
        read(client_socket, write_position, 1);
        if (*write_position == '\n') {
            if (line_length < 3)
                break;
            line_length = 0;
        }
    }
}

/* Everything that goes out before the pixels. */
void send_image_start(void)
{
    adler_low = 1;
    adler_high = 0;

    put_string("HTTP/1.0 200 OK\r\nRefresh: 1\r\n\r\n");
    put_string("\211PNG\r\n\032\n");

    begin_chunk("IHDR");
    put_uint32(IMAGE_WIDTH);
    put_uint32(IMAGE_HEIGHT);
    put_byte(8);                /* bits per channel */
    put_byte(2);                /* colour type: RGB */
    put_byte(0);                /* compression method: deflate */
    put_byte(0);                /* filter method: adaptive */
    put_byte(1);                /* interlace method: Adam7 */
    end_chunk();

    begin_chunk("IDAT");
    put_byte(0x78);             /* zlib header */
    put_byte(0x01);
    end_chunk();
}

/* Everything that goes out after the pixels. */
void send_image_end(void)
{
    /* An empty final deflate block, then the zlib checksum. */
    begin_chunk("IDAT");
    put_byte(1);
    put_uint32(0x0000FFFF);
    put_uint32(adler_high << 16 | adler_low);
    end_chunk();

    begin_chunk("IEND");
    end_chunk();
}

int main()
{
    struct sockaddr_in address = { 0 };
    int server_socket;

    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(SERVER_PORT);
    address.sin_family = AF_INET;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    bind(server_socket, (void *)&address, sizeof address);
    listen(server_socket, LISTEN_BACKLOG);

    for (;;) {
        time_t seconds;
        struct tm *now;
        int pass, x, y;

        client_socket = accept(server_socket, 0, 0);
        skip_request();
        send_image_start();

        seconds = time(0);
        now = localtime(&seconds);
        aim_camera(now->tm_sec);
        build_clock(now);

        /* Each pixel is rendered just before it is sent, so the browser can
         * show the coarse early passes while the later ones are being traced. */
        for (pass = 0; pass < 7; pass++) {
            for (y = adam7_passes[pass].y_start; y < IMAGE_HEIGHT; y += adam7_passes[pass].y_step) {
                begin_row();

                for (x = adam7_passes[pass].x_start; x < IMAGE_WIDTH; x += adam7_passes[pass].x_step) {
                    Vector down_offset;

                    vector_copy(down_offset, pixel_down);
                    vector_scale(down_offset, y - IMAGE_HEIGHT / 2);
                    vector_copy(ray_direction, pixel_right);
                    vector_scale(ray_direction, x - IMAGE_WIDTH / 2);
                    vector_add(ray_direction, down_offset);
                    vector_copy(ray_origin, eye);

                    trace_ray(0);

                    put_colour_component(colour[0]);
                    put_colour_component(colour[1]);
                    put_colour_component(colour[2]);
                }

                end_row();
            }
        }

        send_image_end();
        close(client_socket);
    }
}
