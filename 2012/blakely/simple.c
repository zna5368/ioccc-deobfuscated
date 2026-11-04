/*
 * Render an RPN surface z = f(x, y) as a ray traced, animated GIF.
 *
 *     ./simple "RPN formula" resolution > output.gif
 *
 * No headers are included (just like the original): putchar, atoi and the
 * math functions are implicitly declared and resolved to compiler builtins.
 */

#define MAX_STACK 20

/* Points along a ray where the surface is searched for. */
#define RAY_STEP 4e-3
#define RAY_LENGTH 8

/* Hit coordinate that marks "the ray missed the surface". */
#define MISS -10

/* Scale applied to points on the virtual screen. */
double screen_scale = -1.3 / 8;

/* RPN expression given on the command line. */
char *expression;

/*
 * Evaluation stacks: the value of each entry, and its partial derivatives
 * with respect to x and y.  value_stack[3] and value_stack[4] are also reused
 * by main() as scratch storage.
 */
double value_stack[MAX_STACK], dx_stack[MAX_STACK], dy_stack[MAX_STACK];

/* Partial derivatives dz/dx and dz/dy of the last evaluation. */
double slope_x, slope_y;

/* Camera: point on the virtual screen and the direction of the ray. */
double screen_x, screen_y, screen_z;
double ray_dx, ray_dy, ray_dz;

/* Result of a ray trace: where the surface was hit and how far along. */
double hit_x, hit_y, distance = 0;

/*
 * Evaluate the RPN expression at (x, y).  Returns z and sets slope_x and
 * slope_y to the (approximate) partial derivatives of the surface.
 *
 * A digit is not pushed straight away: it is written just above the top of
 * the stack and marked as pending.  The next operator first commits it.
 */
double evaluate(double x, double y) {
  int top = -1, pending = 0;
  double tmp;
  char *op;

  for (op = expression; *op; op++) {
    switch (*op) {
    case '-':
      top += pending;
      pending = 0;
      value_stack[top - 1] -= value_stack[top];
      dx_stack[top - 1] = dx_stack[top] - dx_stack[top];
      dy_stack[top - 1] = dy_stack[top] - dy_stack[top];
      top--;
      break;
    case 'd': /* duplicate */
      top += pending;
      pending = 0;
      top++;
      value_stack[top] = value_stack[top - 1];
      dx_stack[top] = dx_stack[top - 1];
      dy_stack[top] = dy_stack[top - 1];
      break;
    case '*':
      top += pending;
      pending = 0;
      value_stack[top - 1] *= value_stack[top];
      dx_stack[top - 1] = value_stack[top] * dx_stack[top - 1] +
                          value_stack[top - 1] * dx_stack[top];
      dy_stack[top - 1] = value_stack[top] * dy_stack[top - 1] +
                          value_stack[top - 1] * dy_stack[top];
      top--;
      break;
    case ',': /* separator */
      pending = 0;
      top++;
      break;
    case 't': /* transpose top two elements */
      tmp = value_stack[top - 1];
      value_stack[top - 1] = value_stack[top];
      value_stack[top] = tmp;
      tmp = dx_stack[top];
      dx_stack[top] = dx_stack[top - 1];
      dx_stack[top - 1] = tmp;
      tmp = dy_stack[top];
      dy_stack[top] = dy_stack[top - 1];
      dy_stack[top - 1] = tmp;
      break;
    case '+':
      top += pending;
      pending = 0;
      value_stack[top - 1] += value_stack[top];
      dx_stack[top - 1] = dx_stack[top] + dx_stack[top];
      dy_stack[top - 1] = dy_stack[top] + dy_stack[top];
      top--;
      break;
    case '/':
      top += pending;
      pending = 0;
      value_stack[top - 1] /= value_stack[top];
      dx_stack[top - 1] =
          dx_stack[top - 1] / value_stack[top] -
          value_stack[top - 1] / (value_stack[top] * value_stack[top]) *
              dx_stack[top];
      dy_stack[top - 1] =
          dy_stack[top - 1] / value_stack[top] -
          value_stack[top - 1] / (value_stack[top] * value_stack[top]) *
              dy_stack[top];
      top--;
      break;
    case 'y':
      top += pending;
      pending = 0;
      top++;
      value_stack[top] = y;
      dx_stack[top] = 0;
      dy_stack[top] = 1;
      break;
    case '^': /* power with a constant exponent */
      top += pending;
      pending = 0;
      tmp = value_stack[top - 1];
      value_stack[top - 1] = pow(value_stack[top - 1], value_stack[top]);
      dx_stack[top - 1] = value_stack[top] * dx_stack[top - 1] *
                              pow(tmp, value_stack[top] - 1) +
                          0 * dx_stack[top];
      dy_stack[top - 1] = value_stack[top] * dy_stack[top - 1] *
                              pow(tmp, value_stack[top] - 1) +
                          0 * dy_stack[top];
      top--;
      break;
    /*
     * sin and cos: the derivative is taken from the stack slot just above
     * the top, exactly as the original does.  The "0 * ..." terms are kept
     * so that NaN/infinity values propagate the same way.
     */
    case 's':
      top += pending;
      pending = 0;
      value_stack[top] = sin(value_stack[top]);
      dx_stack[top] = -dx_stack[top] * cos(value_stack[top + 1]) +
                      0 * dx_stack[0];
      dy_stack[top] = -dy_stack[top] * cos(value_stack[top + 1]) +
                      0 * dy_stack[0];
      break;
    case 'c':
      top += pending;
      pending = 0;
      value_stack[top] = cos(value_stack[top]);
      dx_stack[top] = -dx_stack[top] * sin(value_stack[top + 1]) +
                      0 * dx_stack[0];
      dy_stack[top] = -dy_stack[top] * sin(value_stack[top + 1]) +
                      0 * dy_stack[0];
      break;
    case 'x':
      top += pending;
      pending = 0;
      top++;
      value_stack[top] = x;
      dx_stack[top] = 1;
      dy_stack[top] = 0;
      break;
    default:
      if (*op >= '0' && *op <= '9') {
        /* place the digit above the top and leave it pending */
        value_stack[top + 1] = *op - '0';
        dx_stack[top + 1] = 0;
        dy_stack[top + 1] = 0;
        pending = 1;
      }
      break;
    }
  }
  slope_x = dx_stack[0];
  slope_y = dy_stack[0];
  return value_stack[0];
}

/*
 * March along the ray from the screen point until it crosses the surface
 * inside -1 <= x,y <= 1.  Sets hit_x/hit_y (MISS if nothing) and distance.
 */
void trace_ray() {
  double ray_x, ray_y, ray_z, surface_z;
  int side = 0;

  ray_z = screen_z * 8;
  surface_z = evaluate(screen_x * 8, screen_y * 8);
  side = (surface_z - ray_z < 0) ? 1 : -1;
  distance = 0;
  while ((distance += RAY_STEP) < RAY_LENGTH) {
    ray_x = screen_x * 8 + distance * ray_dx;
    ray_y = screen_y * 8 + distance * ray_dy;
    ray_z = screen_z * 8 + distance * ray_dz;
    surface_z = evaluate(ray_x, ray_y);
    if ((surface_z - ray_z) * side > 0) {
      if (ray_x > -1 && ray_x < 1 && ray_y > -1 && ray_y < 1) {
        hit_x = ray_x;
        hit_y = ray_y;
        return;
      } else {
        /* crossed outside the plotted square: keep searching */
        side = -side;
      }
    }
  }
  hit_x = hit_y = MISS;
}

/* Output the two little-endian bytes of the image width or height. */
void put_size(char *size) {
  putchar(size[1]);
  putchar(size[0]);
}

int main(int argc, char **argv) {
  int i, frame;
  char size[2], half;
  double sin_angle = 0, cos_angle = 1, old_sin, step_cos, step_sin, step;
  double row, column, shade, normal_scale;
  double row_axis_x, row_axis_y, col_axis_x, col_axis_y;
  double center_x, center_y;

  expression = argv[1];
  half = atoi(argv[2]) >> 1;
  size[1] = half * 2 & 255; /* low byte of the size */
  size[0] = half >> 9;      /* high byte: always 0 */
  step = 3.1415 / half;

  /* GIF header and logical screen descriptor */
  putchar('G');
  putchar('I');
  putchar('F');
  putchar('8');
  putchar('9');
  putchar('a');
  put_size(size); /* width */
  put_size(size); /* height */
  putchar(0xf6);  /* global color table of 128 entries */

  /* palette: odd entries are a gray ramp, even entries are black */
  for (i = 0; i < 128; i++) {
    int level = i % 2 ? (i / 2) << 2 : 0;
    putchar(level);
    putchar(level);
    putchar(level);
  }
  putchar(0); /* background color */
  putchar(0); /* pixel aspect ratio */

  /* NETSCAPE2.0 application extension: loop count 0x8080 */
  putchar(0x21);
  putchar(0xff);
  putchar(11);
  putchar('N');
  putchar('E');
  putchar('T');
  putchar('S');
  putchar('C');
  putchar('A');
  putchar('P');
  putchar('E');
  putchar('2');
  putchar('.');
  putchar('0');
  putchar(3);
  putchar(1);
  putchar(0x80);
  putchar(0x80);
  putchar(0);

  for (frame = 0; frame <= half * 2; frame++) {
    /* rotate the camera by one step using Taylor series for cos and sin */
    old_sin = sin_angle;
    step_cos = 1 - step * step / 2;
    step_sin = step * (1 - step * step / 6);
    sin_angle = sin_angle * step_cos + cos_angle * step_sin;
    cos_angle = cos_angle * step_cos - old_sin * step_sin;

    col_axis_y = -(cos_angle * cos_angle - sin_angle * sin_angle);
    row_axis_x = -col_axis_y * cos_angle;
    center_x = col_axis_y * sin_angle;
    col_axis_x = sin_angle * cos_angle * 2;
    center_y = -col_axis_x * sin_angle;
    row_axis_y = col_axis_x * cos_angle;

    /* graphic control block (label 0xf6), delay of 10/100 s */
    putchar(0x21);
    putchar(0xf6);
    putchar(4);
    putchar(0);
    putchar(10);
    putchar(0);
    putchar(0);
    putchar(0);

    /* image descriptor */
    putchar(0x2c);
    putchar(0); /* left */
    putchar(0);
    putchar(0); /* top */
    putchar(0);
    put_size(size); /* width */
    put_size(size); /* height */
    putchar(0);     /* no local color table */

    putchar(7); /* LZW minimum code size: codes are 8 bits wide */

    /*
     * Uncompressed image data: each row is split in two sub-blocks, and a
     * clear code (128) is sent often enough that the code size never grows.
     */
    for (row = -half; row < half; row++) {
      putchar(half + 2);
      for (column = -half; column < half; column++) {
        screen_x = (center_x - row_axis_x * row / half -
                    col_axis_x * column / half) *
                   screen_scale;
        screen_y = (center_y - row_axis_y * row / half -
                    col_axis_y * column / half) *
                   screen_scale;
        screen_z = (-cos_angle + sin_angle * row / half) * screen_scale;
        ray_dx = -screen_x + center_x;
        ray_dy = -screen_y + center_y;
        ray_dz = -screen_z - cos_angle;
        trace_ray();

        /* value_stack[3]: ray length, value_stack[4]: normal scale */
        value_stack[3] = sqrt(ray_dx * ray_dx + ray_dy * ray_dy +
                              ray_dz * ray_dz);
        if (hit_x == MISS) {
          shade = 1;
        } else {
          value_stack[4] =
              sqrt(slope_x * slope_x + slope_y * slope_y + 1) * value_stack[3];
          normal_scale = value_stack[4];
          slope_x /= -normal_scale;
          slope_y /= -normal_scale;
          /* cosine between ray and surface normal, faded with distance */
          shade = fabs(slope_x * ray_dx + slope_y * ray_dy +
                       ray_dz / normal_scale) *
                  exp(-distance * distance / 2);
        }

        /* even palette index = black square, odd = shaded square */
        putchar((char)(shade * 63) * 2 +
                ((char)((hit_x + 1) * 6) % 2 + (char)((hit_y + 1) * 6) % 2 ==
                         1
                     ? 1
                     : 0));

        if (column == 0) {
          putchar(128); /* clear code ends first half */
          putchar(row != half - 1 ? half : half + 1);
        }
      }
      putchar(128); /* clear code */
    }
    putchar(129); /* end of information code */
    putchar(0);   /* block terminator */
  }
  putchar(0x3b); /* GIF trailer */
}
