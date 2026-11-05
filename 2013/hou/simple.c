#include <stdio.h>
#include <math.h>

/*
 * Progressive renderer. A scene is a text script of small shaders. Volume
 * shaders fill a voxel grid, then Metropolis light transport keeps mutating
 * light paths and adds each one to the image, which is written to a PPM file
 * every time every pixel has received about one more sample.
 *
 * The program never ends by itself. Stop it when the image looks good enough.
 */

enum {
    BUFFER_SIZE = 1 << 23,
    MAX_SCENE_FILE = 1 << 18,
    GRID_SIDE = 64,
    PRIMARY_SAMPLE_COUNT = 32,
    SAVED_SLOT_COUNT = 192,
    BOUNCE_LIMIT = 5,
    PASSES_PER_SESSION_SAVE = 16,
    /* The voxel walk moves by one of these sizes, in units of 1/64 voxel. */
    FINE_STEP = 1,
    VOXEL_STEP = 64,
    BLOCK_STEP = 512,
    /* Cell values from this number up name a material, not a sub-grid. */
    FIRST_MATERIAL = 64
};

/*
 * Slots of vars[]. A shader variable is stored at the ASCII code of its one
 * character name, so the renderer and the shaders talk through these slots.
 */
enum {
    RAY_EXIT_DISTANCE = 0,          /* three slots: next cell border per axis */
    RAY_INVERSE_DIRECTION = 3,      /* three slots */
    SCRATCH = 7,
    IMAGE_PASS = 8,
    PIXEL_INDEX = 9,
    PATH_BRIGHTNESS = ' ',
    NEW_PATH_WEIGHT = '!',
    SAMPLES_PER_PIXEL = '"',
    CONDITION = '#',
    TOTAL_NEW_PATH_WEIGHT = '$',
    TOTAL_NEW_PATH_BRIGHTNESS = '%',
    QUOTA_COUNTDOWN = '\'',
    PATH_COLOR = 'A',               /* three slots: A, B and C */
    VOXEL_VALUE = 'C',
    IMAGE_WIDTH = 'G',
    IMAGE_HEIGHT = 'H',
    SMALL_MUTATION_CHANCE = 'I',
    PIXEL_QUOTA_CHANCE = 'J',
    ACCEPT_CHANCE_SOURCE = 'O',
    BOUNCE = 'P',
    NEXT_SHADER = 'R',
    HIT_SIGN = 'S',
    HIT_AXIS = 'T',
    RAY_ORIGIN = 'U',               /* three slots: U, V and W */
    RAY_DIRECTION = 'X',            /* three slots: X, Y and Z */
    VOXEL_X = 'X',
    VOXEL_Y = 'Y',
    VOXEL_Z = 'Z',
    BUILTIN_VARIABLES = 'x',        /* the variables that 6:n and 7:n reach */
    PRIMARY_SAMPLES = 127,          /* the shaders start reading at slot 128 */
    SAMPLE_PIXEL_X = 128,
    SAMPLE_PIXEL_Y = 129,
    SAVED_STATE = 254,              /* copy of slots 0 to 191 of the last path */
    /* This slot is reused, because 'O' means nothing to the renderer. */
    ACCEPT_CHANCE = SAVED_STATE + ACCEPT_CHANCE_SOURCE,
    FRAMEBUFFER = 512
};

/* Everything a shader can see. A session file is a dump of this array. */
double vars[BUFFER_SIZE] = {0};

/* The interpreter's position in the scene text. */
char *script;

/* The finished image, written out as a string, so no byte may be zero. */
char image_bytes[BUFFER_SIZE] = {0};

/* Non zero when any voxel in a block of 8x8x8 voxels is not empty. */
char block_occupied[BUFFER_SIZE] = {0};

/*
 * One byte per voxel. Grid number g starts at g * 64 * 64 * 64. Zero means
 * empty, 1 to 63 means "look in sub-grid n" and 64 up is a material.
 */
char voxels[BUFFER_SIZE] = {0};

/* The built in scene, replaced if a scene file can be read. */
char scene[BUFFER_SIZE] =
    /*
     * Table of boxes, seven bytes each: lowest and highest voxel on x, y
     * and z, then the material. Each byte is its value plus 62.
     */
    "@|?|?z#"
    "@|||?z&"
    ">?Sedvf"
    "??Tdeu#"
    ">>Tdeui"
    "??Tdmmf"
    "??[[euf"
    "?}?Oq}e"
    "?}?Oqqf"
    "v}?|q}e"
    "v}?|qqf"
    "?Sj|?{f"
    "?Tjk?{e"
    "FF?Oqq$"
    "RR?Oqq$"
    "^^?Oqq$"
    "jj?Oqq$"
    "vv?|qq$"
    "IOBMqqe"
    "U]BMqqe"
    "aiBMqqe"
    "muBMqqe"
    "EEEJpp%"
    "QQEJpp%"
    "]]EJpp%"
    "iiEJpp%"
    "uuEJpp%"
    "ZnY|{|e"
    "[mZ|{{#"
    "[mZ|||f"
    "kkdr{{%"
    "lkdr{{>"

    /*
     * Box list reader. Each pass tests the voxel against one box of the table
     * above, then repeats until it has read the box whose material is '>'.
     */
    "n]#=1;x=X+1/2;d=0:U;d=d-n;U=U+1;A=d<x;#=#*A;d=0:U;d=d-n;"
    "U=U+1;A=x<d+1;#=#*A;x=Y+1/2;d=0:U;d=d-n;U=U+1;A=d<x;#=#*A;"
    "d=0:U;d=d-n;U=U+1;A=x<d+1;#=#*A;x=Z+1/2;d=0:U;d=d-n;U=U+1;"
    "A=d<x;#=#*A;d=0:U;d=d-n;U=U+1;A=x<d+1;#=#*A;d=0:U;d=d-n;"
    "U=U+1;C=C#d+3*9;#=d;R=0#n"

    /*
     * Volume shader of the top grid. Sets the image size and the Metropolis
     * chances, makes the voxel a wall, then reads the box table from byte 0.
     */
    "A]R=n;G=p*:;#=0<B;G=G*2#G;H=G*3/4;I=4/5;J=1/2;C=p+1;U=0"

    /*
     * Camera shader. Picks a point on the film and on the lens, then starts
     * with no light gathered (A B C) and full throughput (D E F).
     */
    "a]R=0;X=5:5;Y=5:5;X=X-1/2;Y=Y-1/2;x=5:5;y=5:5;x=x-1/2;"
    "y=y-1/2;x=x*8*7;y=y*8*7;X=X*G;Y=Y*H;Z=H*2/3;d=4*;;d=d*p/Z;"
    "X=x+X*d;Y=y+Y*d;Z=Z*d;U=p*p/2-p*2+x;V=p*p/2+p*1-y;W=p*8;A=0;"
    "B=0;C=0;D=1;E=1;F=1"

    /* Diffuse bounce: a cosine weighted direction around the surface normal. */
    "o]R=0;u=5:5;v=5:5;v=v*2;r=3:u;z=1:v;y=1:1/2+v;z=z*r;y=y*r;"
    "x=3:1-u;x=0-x*S;X=6:T;w=x;x=z;z=y;y=w;Y=6:T;w=x;x=z;z=y;y=w;"
    "Z=6:T"

    /* Mirror bounce: flips the direction along the axis that was hit. */
    "5]R=0;x=1;y=1;z=1;#=0-1;#=7:T;X=x*X;Y=y*Y;Z=z*Z"

    /* Glossy surface: a Fresnel style test picks a mirror or diffuse bounce. */
    "f]x=X;y=Y;z=Z;r=3:x*x+z*z+y*y;d=6:T;d=1-d*S/r;d=d*d*d;#=5:5;"
    "#=d<#;R=5#o;d=d#1-d;u=1#u;v=1#v;w=1#w;D=D*d*u;E=E*d*v;"
    "F=F*d*w"

    /* Volume shader of sub-grid 1. */
    "B]R=0;#=X<8*3;C=3#0"

    /* Volume shader of sub-grid 2. */
    "C]R=0;#=X<8*4;C=0#4"

    /* Diffuse material with a repeating star pattern. */
    "1]R=o;d=p*6;x=U+W;x=4:x/d;z=4:V/d;#=3*x-4*z;y=4:#/5;#=#-y*5;"
    "y=1#0;x=x*d+d/2-U+W;z=z*d+d/2-V;r=3:x*x+z*z;#=z;u=2:x;"
    "d=1:u*5;d=d*d*d+3/2;d=d*p;#=r<d;#=#*y;d=2/5#I;D=D*d;E=E*4/7;"
    "d=I#2/5;F=F*d"

    /* Glossy material. */
    "2]R=f;u=1/2;v=1/9;w=v"

    /* Glossy material with a glowing disc. */
    "3]R=f;u=7*9/p;v=u;w=v;x=3*:+8;z=U-x*p+p/2;x=V-x*p-p/2;"
    "r=3:x*x+z*z;#=4<r/p;D=D*I#D;E=E*4/7#E;F=F*2/5#F;d=1/5#0;"
    "A=A+D*d;B=B+E*d;C=C+F*d"

    /* Coloured mirror. */
    "4]R=5;D=D*I;E=E*4/7;F=F*2/5"

    /* Light source. Adds its light to the path, and P=9 ends the path. */
    "6]R=0;#=P;d=2#8*2;A=A+D*d;B=B+E*d;C=C+F*d*I;P=9"

    /* Coloured diffuse material. */
    "7]R=o;D=D/7;E=E/3;F=F/7"

    /* Volume shader of sub-grid 3. */
    "D]R=0;#=8<Y;C=0#3;#=4:3*X-4*Z;y=4:#/7;#=#-y*7;C=7#C"

    /* The second ']' is replaced by a digit to name the session file. */
    ";]]luna.ppm";

/* Address of the text after "name]" for each shader name. */
char *labels[512] = {0};

/* Status line, which changes once the image file has been written once. */
char *status_format = "\r%d\topen\t%s";

/* Position of the ray in the grid, in units of 1/64 voxel. */
int ray_cell[3] = {0};

/*
 * Direction of the ray on each axis, +1 or -1. Larger than three because
 * hit_axis holds the argument count until the first ray has been walked.
 */
int ray_step[512] = {0};

/* The axis whose cell border the ray crossed last. */
int hit_axis;

/* How many more samples each pixel may receive this round. */
int pixel_quota[BUFFER_SIZE] = {0};

/* Slot of the last number handed to a shader by the 5:n built in. */
int current_sample;

int rng_state = 1;

/* Pseudo random number between 0 and 1. */
double next_random(void)
{
    int mixed;

    mixed = rng_state = rng_state * 69069 + 1;
    rng_state = rng_state * 69069 + 1;
    vars[SCRATCH] = mixed ^= rng_state >> 16;
    return (1 + vars[SCRATCH] / 2147483648.0) / 2;
}

/* The ':' operator. The left operand picks the function. */
double call_builtin(double function, double argument)
{
    /* Written with ! so that a function number of NaN selects number 0. */
    if (!(function > 0))
        return scene[(int)argument];
    if (!(function > 1))
        return sin(argument * 3.1416);
    if (!(function > 2))
        return atan2(argument, vars[CONDITION]) / 3.1416;
    if (!(function > 3))
        return argument > 0 ? sqrt(argument) : 0;
    if (!(function > 4))
        return floor(argument);
    if (!(function > 5))
        return vars[++current_sample];
    if (!(function > 6))
        return vars[(int)(argument + BUILTIN_VARIABLES)];
    return vars[(int)(argument + BUILTIN_VARIABLES)] = vars[CONDITION];
}

/*
 * Evaluate one shader expression. There are no numbers or brackets, only one
 * character variables and these operators, loosest binding first. Each level
 * parses the tighter levels on its left, then calls itself for its right, so
 * every operator groups to the right: a-b-c means a-(b-c), and because '*'
 * binds tighter than '/', a/b*c means a/(b*c).
 */
double evaluate(const char *operators)
{
    char operator = *operators;
    double left, right;

    if (operator)
        left = evaluate(operators + 1);
    else
        left = vars[(int)*script++];

    if (*script != operator)
        return left;
    script++;
    right = vars[SCRATCH] = evaluate(operators);

    switch (operator) {
    case ':':
        return call_builtin(left, right);
    case '#':
        return vars[CONDITION] ? right : left;
    case '<':
        return left < right;
    case '-':
        return left - right;
    case '+':
        return left + right;
    case '/':
        return left / right;
    case '*':
        return left * right;
    }
    return left;
}

/*
 * Run shaders, starting with the one named by NEXT_SHADER. Scene text is a
 * list of statements "?v=expression" where ? is any one character. When the
 * next text does not have that shape, the shader is over, and NEXT_SHADER
 * names the next one. A name with no label returns to the renderer.
 *
 * The search starts at byte 0 of the scene, which is the table of boxes, so
 * the first test fails and the jump to the first shader is taken.
 */
void run_shaders(void)
{
    int variable;

    script = scene;
    for (;;) {
        if (script[2] != '=') {
            script = labels[(int)(vars[NEXT_SHADER] + '0')];
            if (!script)
                return;
        }
        variable = script[1];
        script += 3;
        vars[variable] = evaluate(":#<-+/*");
    }
}

/*
 * Find the shader labels. Every character that appears in the scene also
 * becomes a variable holding its ASCII code minus 48, which is the only way
 * that shaders can get a number: the variable '5' holds 5.
 */
void read_scene_text(void)
{
    char *cursor;

    for (cursor = scene; *cursor++; vars[(int)*cursor] = *cursor - '0')
        if (*cursor == ']')
            labels[(int)cursor[-1]] = cursor;
}

/*
 * Voxel number is grid, x, y, z with six bits for each coordinate. Keeping
 * the top three bits of each gives the 8x8x8 voxel block holding that voxel.
 */
int block_of(int voxel)
{
    return voxel >> 3 & 07770707;
}

/* Run the volume shaders A, B, C... for each voxel of their grid. */
void build_voxel_grids(void)
{
    int shader, voxel = 0;

    for (shader = 'A' - '0'; labels[shader + '0']; shader++)
        for (vars[VOXEL_X] = 0; vars[VOXEL_X] < GRID_SIDE; vars[VOXEL_X]++)
            for (vars[VOXEL_Y] = 0; vars[VOXEL_Y] < GRID_SIDE; vars[VOXEL_Y]++)
                for (vars[VOXEL_Z] = 0; vars[VOXEL_Z] < GRID_SIDE; vars[VOXEL_Z]++) {
                    vars[NEXT_SHADER] = shader;
                    run_shaders();
                    voxels[voxel] = vars[VOXEL_VALUE];
                    block_occupied[block_of(voxel)] |= voxels[voxel];
                    voxel++;
                }
}

/* The voxel of the top grid that holds the ray. */
int voxel_under_ray(void)
{
    int axis, voxel = 0;

    for (axis = 0; axis < 3; axis++)
        voxel = voxel * GRID_SIDE + (ray_cell[axis] >> 6);
    return voxel;
}

/* Distance along the ray to the next cell border on one axis. */
void find_cell_border(int axis, int cell_size)
{
    vars[RAY_EXIT_DISTANCE + axis] =
        ((ray_cell[axis] / cell_size + (ray_step[axis] > 0)) * cell_size
         - vars[RAY_ORIGIN + axis]) * vars[RAY_INVERSE_DIRECTION + axis];
}

/* Set up the grid walk for the ray that the shaders just described. */
void start_ray(void)
{
    int axis;

    for (axis = 0; axis < 3; axis++) {
        vars[RAY_INVERSE_DIRECTION + axis] = 1 / vars[RAY_DIRECTION + axis];
        ray_step[axis] = (vars[RAY_INVERSE_DIRECTION + axis] > 0) * 2 - 1;
        ray_cell[axis] = vars[RAY_ORIGIN + axis];
        find_cell_border(axis, FINE_STEP);
    }
}

/*
 * 3D digital differential analyzer. Step from cell to cell, always crossing
 * the nearest border, until a voxel names a shader. Empty space is skipped
 * with big steps, and the step shrinks again near anything solid. A zero
 * direction makes the distance on that axis infinite, which skips that axis.
 */
void walk_to_surface(void)
{
    int cell_size = FINE_STEP;
    int axis, wanted_size, voxel, value;

    while (!vars[NEXT_SHADER]) {
        hit_axis = 0;
        if (vars[RAY_EXIT_DISTANCE + hit_axis] > vars[RAY_EXIT_DISTANCE + 1])
            hit_axis = 1;
        if (vars[RAY_EXIT_DISTANCE + hit_axis] > vars[RAY_EXIT_DISTANCE + 2])
            hit_axis = 2;
        vars[SCRATCH] = vars[RAY_EXIT_DISTANCE + hit_axis];

        /* Cross the border and snap the position to the new cell. */
        ray_cell[hit_axis] += ray_step[hit_axis] * cell_size;
        vars[RAY_EXIT_DISTANCE + hit_axis] += ray_step[hit_axis] * cell_size
            * vars[RAY_INVERSE_DIRECTION + hit_axis];
        ray_cell[hit_axis] &= -cell_size;
        ray_cell[hit_axis] += ray_step[hit_axis] & (cell_size - 1);

        voxel = voxel_under_ray();
        if (!block_occupied[block_of(voxel)])
            wanted_size = BLOCK_STEP;
        else if (!voxels[voxel])
            wanted_size = VOXEL_STEP;
        else
            wanted_size = FINE_STEP;

        /* After a resize the other axes need a fresh position and border. */
        if (cell_size != wanted_size) {
            cell_size = cell_size < wanted_size ? wanted_size : FINE_STEP;
            for (axis = 0; axis < 3; axis++) {
                if (axis != hit_axis)
                    ray_cell[axis] = vars[RAY_ORIGIN + axis]
                        + vars[SCRATCH] * vars[RAY_DIRECTION + axis];
                find_cell_border(axis, cell_size);
            }
            voxel = voxel_under_ray();
        }

        value = voxels[voxel];
        if (value) {
            /* A sub-grid uses the low six bits of the position. */
            if (value < FIRST_MATERIAL) {
                for (axis = 0; axis < 3; axis++)
                    value = value * GRID_SIDE + (ray_cell[axis] & 63);
                value = voxels[value] + FIRST_MATERIAL;
            }
            vars[NEXT_SHADER] = value - FIRST_MATERIAL;
        }
    }
}

/*
 * Follow one light path. Bounce -1 only runs the camera shader. After that
 * each bounce walks to a surface, moves the origin there and runs its shader.
 * With no walk, the origin moves by whatever is left in SCRATCH. That happens
 * on the camera bounce, whose shader then sets a new origin, and after a voxel
 * names a shader that does not exist, which leaves nothing to run for the rest
 * of the path.
 */
void trace_path(void)
{
    int axis;

    for (vars[BOUNCE] = -1; vars[BOUNCE] < BOUNCE_LIMIT; vars[BOUNCE]++) {
        walk_to_surface();
        for (axis = 0; axis < 3; axis++)
            vars[RAY_ORIGIN + axis] += vars[SCRATCH] * vars[RAY_DIRECTION + axis];
        vars[HIT_SIGN] = ray_step[hit_axis];
        vars[HIT_AXIS] = hit_axis;
        run_shaders();
        start_ray();
    }
}

/* Remember the current path as the one that future mutations start from. */
void keep_current_path(void)
{
    int slot;

    for (slot = 0; slot < SAVED_SLOT_COUNT; slot++)
        vars[SAVED_STATE + slot] = vars[slot];
}

/* Make new random numbers. A path is defined entirely by these numbers. */
void pick_new_samples(void)
{
    int i;

    for (i = 0; i < PRIMARY_SAMPLE_COUNT; i++)
        vars[PRIMARY_SAMPLES + i] = next_random();
    vars[NEW_PATH_WEIGHT] = 1. / 2;
}

/* Replace the new numbers by a small random change to the last path's. */
void mutate_kept_samples(void)
{
    int i, whole_part;

    /* A random number to the fourth power makes most changes tiny. */
    vars[SCRATCH] = next_random();
    vars[SCRATCH] *= vars[SCRATCH];
    vars[SCRATCH] *= vars[SCRATCH];
    for (i = 0; i < PRIMARY_SAMPLE_COUNT; i++) {
        /* Adding 1 and dropping the whole part wraps around from 0 to 1. */
        whole_part = vars[PRIMARY_SAMPLES + i] =
            (vars[PRIMARY_SAMPLES + i] - vars[NEW_PATH_WEIGHT]) * vars[SCRATCH]
            + 1 + vars[SAVED_STATE + PRIMARY_SAMPLES + i];
        vars[PRIMARY_SAMPLES + i] -= whole_part;
    }
    vars[NEW_PATH_WEIGHT] = 0;
}

/* The pixel that the first two random numbers point at. */
int pixel_from_samples(void)
{
    int row, pixel;

    row = vars[SAMPLE_PIXEL_Y] * vars[IMAGE_HEIGHT];
    pixel = row * vars[IMAGE_WIDTH] + vars[SAMPLE_PIXEL_X] * vars[IMAGE_WIDTH];
    vars[PIXEL_INDEX] = pixel;
    return pixel;
}

/*
 * Lens stratification. Each pixel gets a quota of three samples per round,
 * and a pixel that has used up its quota is replaced by a random one, which
 * stops the mutations from spending all their time on the brightest pixels.
 */
void spend_pixel_quota(int pixel)
{
    int i, pixel_count = vars[IMAGE_HEIGHT] * vars[IMAGE_WIDTH];

    if (--vars[QUOTA_COUNTDOWN] < pixel_count) {
        vars[QUOTA_COUNTDOWN] += 3 * pixel_count;
        for (i = 0; i < pixel_count; i++)
            pixel_quota[i] += 3;
    }
    while (!pixel_quota[pixel]) {
        vars[SAMPLE_PIXEL_X] = next_random();
        vars[SAMPLE_PIXEL_Y] = next_random();
        pixel = pixel_from_samples();
    }
    pixel_quota[pixel]--;
}

/*
 * Metropolis acceptance. Both the kept path and the new path are added to the
 * image, weighted by the chance of accepting the new one. Dividing by the old
 * brightness gives infinity or NaN for the first path, and neither one passes
 * the "less than 1" test, so that path is always accepted.
 */
void add_path_to_image(void)
{
    int channel, slot;

    vars[PATH_BRIGHTNESS] = 0;
    for (channel = 0; channel < 3; channel++)
        if (vars[PATH_BRIGHTNESS] < vars[PATH_COLOR + channel])
            vars[PATH_BRIGHTNESS] = vars[PATH_COLOR + channel];

    vars[TOTAL_NEW_PATH_WEIGHT] += vars[NEW_PATH_WEIGHT];
    vars[TOTAL_NEW_PATH_BRIGHTNESS] += vars[NEW_PATH_WEIGHT] * vars[PATH_BRIGHTNESS];
    vars[SAMPLES_PER_PIXEL] += 1. / vars[IMAGE_WIDTH] / vars[IMAGE_HEIGHT];

    vars[ACCEPT_CHANCE] = vars[PATH_BRIGHTNESS] / vars[SAVED_STATE + PATH_BRIGHTNESS];
    if (!(vars[ACCEPT_CHANCE] < 1))
        vars[ACCEPT_CHANCE] = 1;

    for (channel = 0; channel < 3; channel++) {
        /* Every path counts the same, so only its colour is stored. */
        vars[PATH_COLOR + channel] /= vars[PATH_BRIGHTNESS] + !vars[PATH_BRIGHTNESS];

        slot = FRAMEBUFFER + vars[SAVED_STATE + PIXEL_INDEX] * 3 + channel;
        vars[slot] += vars[SAVED_STATE + PATH_COLOR + channel] * (1 - vars[ACCEPT_CHANCE]);

        slot = FRAMEBUFFER + vars[PIXEL_INDEX] * 3 + channel;
        vars[slot] += vars[PATH_COLOR + channel] * vars[ACCEPT_CHANCE];
    }
}

/* Convert the framebuffer to bytes and write the PPM file. */
void write_image(char *image_path)
{
    FILE *file;
    int i, byte, pixel_count = vars[IMAGE_HEIGHT] * vars[IMAGE_WIDTH];

    for (i = 0; i < 3 * pixel_count; i++) {
        /*
         * Scale by the average path brightness, then apply a gamma of 2.
         * Bytes run from 1 to 255 so that the image can be printed with %s.
         */
        vars[SCRATCH] = sqrt(vars[FRAMEBUFFER + i] * vars[TOTAL_NEW_PATH_BRIGHTNESS]
                             / vars[TOTAL_NEW_PATH_WEIGHT] / vars[SAMPLES_PER_PIXEL]);
        vars[SCRATCH] = vars[SCRATCH] < 1 ? vars[SCRATCH] : 1;
        byte = vars[SCRATCH] * 254 + 1;
        image_bytes[i] = byte;
    }
    file = fopen(image_path, "wb");
    fprintf(file, "P6\n#\n%d\t%d\n255\n%s",
            (int)vars[IMAGE_WIDTH], (int)vars[IMAGE_HEIGHT], image_bytes);
    fclose(file);
}

/* Save the whole renderer state so a later run can carry on from it. */
void save_session(char *session_path)
{
    FILE *file = fopen(session_path, "wb");

    fwrite(vars, sizeof vars, 1, file);
    fclose(file);
}

/* Carry on from an earlier run of the same scene and options, if any. */
void load_session(char *session_path)
{
    FILE *file = fopen(session_path, "rb");

    if (file) {
        fread(vars, sizeof vars, 1, file);
        fclose(file);
        vars[QUOTA_COUNTDOWN] = 0;
    }
}

/* Write the image each time every pixel has about one more sample. */
void report_progress(char *session_path)
{
    char *image_path = session_path + 1;
    int pass;

    if (vars[IMAGE_PASS] + 1 < vars[SAMPLES_PER_PIXEL]) {
        write_image(image_path);
        pass = ++vars[IMAGE_PASS];
        printf(status_format, pass, image_path);
        status_format = "\r%d\treopen\t%s";
        fflush(stdout);
        if (pass % PASSES_PER_SESSION_SAVE == 0)
            save_session(session_path);
    }
}

int main(int argc, char **argv)
{
    FILE *scene_file;
    char *session_path;
    int pixel;

    printf("please\twait...\n");

    if (argc > 1) {
        scene_file = fopen(argv[1], "rb");
        if (scene_file)
            scene[fread(scene, 1, MAX_SCENE_FILE, scene_file)] = 0;
    }
    read_scene_text();

    /*
     * The scene text ends with "]]image.ppm". The second ']' becomes a digit
     * that depends on the argument count, which gives the session file name.
     */
    session_path = labels[']'];
    *session_path = argc + '1';

    /* An option switches a feature off by zeroing the variable it starts with. */
    hit_axis = argc - 1;
    if (hit_axis)
        vars[(int)*argv[hit_axis]] = 0;

    build_voxel_grids();
    load_session(session_path);

    for (;;) {
        current_sample = PRIMARY_SAMPLES;
        vars[NEXT_SHADER] = 'a' - '0';

        if (next_random() < vars[ACCEPT_CHANCE])
            keep_current_path();

        pick_new_samples();
        if (next_random() < vars[SAVED_STATE + SMALL_MUTATION_CHANCE])
            mutate_kept_samples();

        pixel = pixel_from_samples();
        if (next_random() < vars[SAVED_STATE + PIXEL_QUOTA_CHANCE])
            spend_pixel_quota(pixel);

        trace_path();
        add_path_to_image();
        report_progress(session_path);
    }
}
