#include <stdio.h>
#include <SDL.h>

/* Limits of the object table */
#define MAX_OBJECTS 333

/* Object behaviour flags (CLASSFLAGS column of the level file) */
#define FLAG_ENEMY        1
#define FLAG_BLOCK        2
#define FLAG_OBJECT_BLOCK 4
#define FLAG_POWERUP      8
#define FLAG_END          16
#define FLAG_ZOOM         32
#define FLAG_DESTROY      64
#define FLAG_DESTROYUP    128

/* Sprite sheet layout */
#define SPRITES_PER_ROW     8
#define SPRITE_PLAYER       0
#define SPRITE_SUPER_PLAYER 8
#define SPRITE_PLAYER_DEAD  24
#define SPRITE_PLAYER_WON   32
#define SUPER_WON_OFFSET    8
#define LEFT_FACING_COLUMNS 4
#define JUMP_SPRITE_SOURCE_X 48 /* pixels, not scaled by the sprite size */

/* Player physics, in pixels per movement step */
#define RUN_STEPS_PER_FRAME  4
#define FALL_STEPS_PER_FRAME 3
#define JUMP_VELOCITY        (-9)
#define STOMP_VELOCITY       (-6)
#define BLOCKED_FALL_VELOCITY 1
#define GRAVITY              2
#define MAX_FALL_VELOCITY    2

/* Other game rules */
#define OBJECT_FALL_SPEED    2
#define ENEMY_WALK_STEPS     20
#define SPAWNED_WALK_STEP    2
#define INVINCIBLE_FRAMES    30
#define FLICKER_PERIOD       3
#define END_DELAY_FRAMES     50
#define FRAME_DELAY_MS       60
#define EXIT_PLAYER_DIED     (-1)

/* Surface flags value kept from the original */
#define SPRITE_SURFACE_FLAGS 0x8000

/* Program arguments */
enum {
    ARG_WINDOW_WIDTH = 1,
    ARG_WINDOW_HEIGHT,
    ARG_LEVEL_WIDTH,
    ARG_LEVEL_HEIGHT,
    ARG_SPRITES_WIDTH,
    ARG_SPRITES_HEIGHT,
    ARG_SPRITES_FILE,
    ARG_MUSIC_FILE,
    ARG_SKY_COLOR
};

typedef struct {
    int active;
    int x;
    int y;
    int sprite;
    int flags;
    int param0; /* enemies and power-ups: walking step, blocks: spawned flags */
    int param1; /* enemies: steps walked, blocks: spawned sprite */
} GameObject;

static GameObject objects[MAX_OBJECTS];
static GameObject *player;

static int level_width, level_height;
static int window_width, window_height;
static int sprite_size;
static int sky_color;

static int player_x, player_y, player_height;
static int facing = 1;
static int velocity_x;
static int velocity_y = 5;
static int on_ground;
static int is_super;
static int invincible_frames;

static int camera_x, camera_y;
static int frame_count;
static int end_countdown;
static int exit_code;

static SDL_Surface *screen, *sprites;

static Uint8 *music_data;
static Uint32 music_length;
static int music_position;

static void play_music(void *userdata, Uint8 *stream, int length);

static SDL_AudioSpec audio_request = {
    .freq = 8000,
    .format = AUDIO_U8,
    .channels = 1,
    .samples = 256,
    .callback = play_music
};
static SDL_AudioSpec music_spec;

/* Freeze the controls and leave the game once the countdown runs out */
static void start_end_countdown(void)
{
    end_countdown = END_DELAY_FRAMES;
    velocity_x = 0;
    on_ground = 1;
}

/* Does an active, non-decorative object overlap the given box? */
static int collides(const GameObject *object, int x, int y, int height)
{
    return object->active && object->flags &&
           x < object->x + sprite_size && y < object->y + sprite_size &&
           object->x < x + sprite_size && object->y < y + height;
}

/* Put a copy of the template in the first free slot of the object table */
static GameObject *spawn_object(const GameObject *template)
{
    int index;

    for (index = 0; index < MAX_OBJECTS; index++) {
        if (!objects[index].active) {
            objects[index] = *template;
            objects[index].active = 1;
            return &objects[index];
        }
    }
    return NULL;
}

/* Move the player one step and react to the first object that gets in the way */
static void move_player(int dx, int dy)
{
    int index, flags, is_enemy;
    int max_x = level_width - sprite_size;
    GameObject *object;

    player_x += dx;
    player_y += dy;
    if (player_x < 0)
        player_x = 0;
    else if (player_x > max_x)
        player_x = max_x;

    for (index = 0; index < MAX_OBJECTS; index++) {
        object = &objects[index];
        if (!collides(object, player_x, player_y, player_height))
            continue;

        flags = object->flags;
        is_enemy = flags & FLAG_ENEMY;
        if (invincible_frames && is_enemy)
            continue;

        player_x -= dx;
        player_y -= dy;

        if (flags & FLAG_POWERUP) {
            object->active = 0;
            is_super = 1;
            player->sprite = SPRITE_SUPER_PLAYER;
            if (flags & FLAG_ZOOM) {
                player_y -= sprite_size;
                player_height = sprite_size * 2;
            }
        }

        if ((flags & FLAG_END) && !end_countdown) {
            player->sprite = SPRITE_PLAYER_WON +
                             (player_height > sprite_size ? SUPER_WON_OFFSET : 0);
            start_end_countdown();
            object->active = 0;
        }

        if (((flags & FLAG_DESTROYUP) && dy && velocity_y < 0) ||
            (flags & FLAG_DESTROY))
            object->active = 0;

        if (is_enemy && !end_countdown) {
            if (dy && velocity_y > 0) {
                /* Stomped: switch to the squashed sprite and bounce */
                object->sprite--;
                object->flags = 0;
                dy = 0;
                velocity_y = STOMP_VELOCITY;
            } else if (is_super) {
                /* Hit while super: lose the power but survive */
                player->sprite = SPRITE_PLAYER;
                if (player_height > sprite_size) {
                    player_height = sprite_size;
                    player_y += sprite_size;
                }
                invincible_frames = INVINCIBLE_FRAMES;
                is_super = 0;
            } else {
                player->sprite = SPRITE_PLAYER_DEAD;
                start_end_countdown();
                exit_code = EXIT_PLAYER_DIED;
            }
        }

        if ((flags & FLAG_OBJECT_BLOCK) && dy && velocity_y < 0) {
            /* Hit from below: the block releases the object it holds */
            GameObject released = {
                .x = object->x,
                .y = object->y - sprite_size,
                .sprite = object->param1,
                .flags = object->param0,
                .param0 = rand() % 2 ? 1 : -1,
                .param1 = SPAWNED_WALK_STEP
            };
            object->sprite++;
            object->flags = FLAG_BLOCK;
            spawn_object(&released);
        }

        if (dy)
            velocity_y = BLOCKED_FALL_VELOCITY;
        break;
    }
}

/* Keep the view centred on the player without leaving the level */
static int camera_offset(int position, int level_size, int view_size)
{
    int half_view = view_size / 2;

    if (position > level_size - half_view)
        return view_size - level_size;
    if (position > half_view)
        return half_view - position;
    return 0;
}

/* Move an enemy or power-up one step unless a block is in the way */
static void move_object(int index, int dx, int dy)
{
    GameObject *object = &objects[index];
    int other, blocked = 0;

    object->x += dx;
    object->y += dy;

    for (other = 0; other < MAX_OBJECTS; other++) {
        if (other != index &&
            collides(&objects[other], object->x, object->y, sprite_size) &&
            (objects[other].flags & (FLAG_BLOCK | FLAG_OBJECT_BLOCK))) {
            blocked = 1;
            break;
        }
    }

    if (blocked) {
        if (dy)
            object->y -= dy;
        if (dx) {
            object->x -= dx;
            /* Only reverses a rightward step: a step of -1 multiplies by 1 */
            object->param0 *= -1 * dx;
        }
    }
}

/* Update an object for this frame and draw it */
static void update_and_draw_object(int index)
{
    GameObject *object = &objects[index];
    int flags = object->flags;
    int draw_x, draw_y, source_x, source_y, draw_height;

    if (flags & (FLAG_ENEMY | FLAG_POWERUP)) {
        move_object(index, object->param0, 0);
        move_object(index, 0, OBJECT_FALL_SPEED);
    }

    /* The enemy walking step below only shows up on the next frame */
    draw_x = object->x;
    draw_y = object->y;
    source_x = (object->sprite % SPRITES_PER_ROW) * sprite_size;
    source_y = (object->sprite / SPRITES_PER_ROW) * sprite_size;

    if (flags & FLAG_ENEMY) {
        object->x += object->param0;
        if (++object->param1 > ENEMY_WALK_STEPS) {
            object->param0 *= -1;
            object->param1 = 0;
        }
        if (frame_count % 2)
            source_x += sprite_size;
    }

    draw_height = sprite_size;
    if (object == player) {
        draw_height = player_height;
        if (!end_countdown) {
            if (velocity_x)
                source_x += sprite_size * (frame_count % 2);
            if (!on_ground)
                source_x = JUMP_SPRITE_SOURCE_X;
            if (facing < 0)
                source_x += sprite_size * LEFT_FACING_COLUMNS;
        }
    }

    /* The player flickers while invincible */
    if (object != player ||
        !(invincible_frames && invincible_frames % FLICKER_PERIOD == 0)) {
        SDL_Rect source = { source_x, source_y, sprite_size, draw_height };
        SDL_Rect target = { camera_x + draw_x, camera_y + draw_y,
                            sprite_size, draw_height };
        SDL_BlitSurface(sprites, &source, screen, &target);
    }
}

/* Advance the game by one frame and draw it */
static void run_frame(void)
{
    int previous_y = player_y;
    int step, index;

    SDL_FillRect(screen, NULL, sky_color);

    for (step = 0; step < RUN_STEPS_PER_FRAME; step++)
        move_player(velocity_x, 0);
    for (step = 0; step < FALL_STEPS_PER_FRAME; step++)
        move_player(0, velocity_y);

    /* Falling out of the level ends the game, but exit_code stays 0 */
    if (player_y > level_height && !end_countdown)
        start_end_countdown();

    on_ground = previous_y == player_y;
    player->x = player_x;
    player->y = player_y;
    camera_x = camera_offset(player_x, level_width, window_width);
    camera_y = camera_offset(player_y, level_height, window_height);

    for (index = 0; index < MAX_OBJECTS; index++)
        if (objects[index].active)
            update_and_draw_object(index);

    velocity_y += GRAVITY;
    if (velocity_y > MAX_FALL_VELOCITY)
        velocity_y = MAX_FALL_VELOCITY;

    frame_count++;
    if (invincible_frames)
        invincible_frames--;
    if (end_countdown && --end_countdown == 0)
        exit(exit_code);
}

static int int_argument(const char *argument)
{
    return strtol(argument, NULL, 0);
}

/* SDL audio callback: stream the music, looping at the end */
static void play_music(void *userdata, Uint8 *stream, int length)
{
    int available = (int)music_length - music_position;

    if (available > length)
        available = length;
    SDL_MixAudio(stream, music_data + music_position, available, SDL_MIX_MAXVOLUME);
    music_position += available;
    if (music_position >= (int)music_length)
        music_position = 0;
}

/* Read objects from stdin until the row that ends the level */
static void load_level(void)
{
    GameObject row;
    GameObject *object;

    for (;;) {
        scanf("%d %d %d %d %d %d ", &row.x, &row.y, &row.sprite, &row.flags,
              &row.param0, &row.param1);
        if (row.param1 < 0)
            break;

        object = spawn_object(&row);

        /* An object with no flags is scenery, and the last one is the player */
        if (!row.flags) {
            player = object;
            player_x = row.x;
            player_y = row.y;
        }
    }
}

static void handle_key(int key, int released)
{
    if (key == SDLK_LEFT) {
        if (released) {
            velocity_x = 0;
        } else {
            facing = -1;
            velocity_x = -1;
        }
    }
    if (key == SDLK_RIGHT) {
        if (released) {
            velocity_x = 0;
        } else {
            facing = 1;
            velocity_x = 1;
        }
    }
    if (key == SDLK_SPACE && !released && on_ground)
        velocity_y = JUMP_VELOCITY;
    if (key == SDLK_ESCAPE)
        exit(0);
}

int main(int argc, char **argv)
{
    int endian_probe = 1;
    Uint32 red_mask = 0x000000ff, green_mask = 0x0000ff00;
    Uint32 blue_mask = 0x00ff0000, alpha_mask = 0xff000000;
    int sprites_width = int_argument(argv[ARG_SPRITES_WIDTH]);
    int sprites_height = int_argument(argv[ARG_SPRITES_HEIGHT]);
    SDL_Event event;

    player_height = sprite_size = sprites_width / SPRITES_PER_ROW;
    window_width = int_argument(argv[ARG_WINDOW_WIDTH]);
    window_height = int_argument(argv[ARG_WINDOW_HEIGHT]);
    level_width = int_argument(argv[ARG_LEVEL_WIDTH]);
    level_height = int_argument(argv[ARG_LEVEL_HEIGHT]);
    sky_color = int_argument(argv[ARG_SKY_COLOR]);

    SDL_Init(SDL_INIT_EVERYTHING);

    /* The image is raw RGBA bytes, so the masks depend on the byte order */
    if (!*(char *)&endian_probe) {
        red_mask = 0xff000000;
        green_mask = 0x00ff0000;
        blue_mask = 0x0000ff00;
        alpha_mask = 0x000000ff;
    }

    screen = SDL_SetVideoMode(window_width, window_height, 0, 0);
    sprites = SDL_CreateRGBSurface(SPRITE_SURFACE_FLAGS, sprites_width, sprites_height,
                                   32, red_mask, green_mask, blue_mask, alpha_mask);
    fread(sprites->pixels, sprites_width * sprites_height * 4, 1,
          fopen(argv[ARG_SPRITES_FILE], "r"));

    SDL_OpenAudio(&audio_request, NULL);
    SDL_LoadWAV(argv[ARG_MUSIC_FILE], &music_spec, &music_data, &music_length);
    SDL_PauseAudio(0);

    load_level();

    for (;;) {
        while (SDL_PollEvent(&event)) {
            int released = event.type == SDL_KEYUP;

            if (!end_countdown && (released || event.type == SDL_KEYDOWN))
                handle_key(event.key.keysym.sym, released);
        }
        run_frame();
        SDL_Flip(screen);
        SDL_Delay(FRAME_DELAY_MS);
    }
}
