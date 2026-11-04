#include <SDL.h>
#include <stdlib.h>

#include <sys/mman.h>

#define WIDTH 400
#define HEIGHT 300
#define ITERATIONS 128

/*
 * One block of read/write/execute memory is split into four regions of
 * REGION_DOUBLES doubles (4096 bytes) each:
 *
 *   constants  numbers that appear in the formulas
 *   spill      values pushed out of the 8-register x87 stack
 *   vars       variables a-z: vars[v] is the real part, vars[v + 26] imag
 *   code       the generated machine code
 */
#define REGION_DOUBLES 512
#define MEMORY_SIZE (REGION_DOUBLES * 32)

#define IMAG 26 /* offset from a variable's real part to its imaginary part */

/* Operator characters are replaced by these codes before parsing.  An
 * operator's precedence level is its code divided by two. */
enum {
  OP_END_STATEMENT = 1, /* ';' */
  OP_LESS = 2,          /* '<' level 1 */
  OP_GREATER = 3,       /* '>' level 1 */
  OP_ADD = 4,           /* '+' level 2 */
  OP_SUB = 5,           /* '-' level 2 */
  OP_MUL = 6,           /* '*' level 3 */
  OP_DIV = 7,           /* '/' level 3 */
  OP_NONE = 9           /* before the first operand */
};

#define LEVEL_COMPARE 1
#define LEVEL_UNARY 4
#define LEVEL_PRIMARY 5

/* A pixel's offset to where its colour was really calculated */
struct sample {
  float dx, dy, dist;
};

SDL_Event event;
SDL_Surface *screen;
struct sample samples[HEIGHT][WIDTH][2];
char reused_color[HEIGHT][WIDTH];

double pixel_step_x = 2 / (WIDTH - 1.), pixel_step = 2 / (HEIGHT - 1.);
double center_re, center_im_neg, scale = 2;

/* code generator state */
double *next_constant, *vars, *spill_top;
char *code_base, *code;
Sint32 *iterations; /* immediate operand of "mov edx, ITERATIONS" */
int fpu_depth;      /* values currently held in the x87 register stack */

void emit1(int b) { *code++ = b; }

void emit2(int b1, int b2) {
  *code++ = b1;
  *code++ = b2;
}

/*
 * Memory is addressed as [rcx + disp32].  The prologue puts the address of
 * code_base + 5 in rcx, so the displacement is relative to that.
 */
void emit_address(void *target) {
  *(Sint32 *)code = (char *)target - code_base - 5;
  code += 4;
}

/* About to push a value.  If all 8 registers are used, the bottom value is
 * stored in the spill area first. */
void fpu_push(void) {
  if (fpu_depth > 7) {
    emit2(0xD9, 0xF6); /* fdecstp: bottom value becomes st0 */
    emit2(0xDD, 0x99); /* fstp qword [spill]           */
    emit_address(spill_top++);
  } else
    fpu_depth++;
}

/* Make sure at least n values are in registers, reloading spilled ones
 * underneath the values that are already there. */
void fpu_need(int n) {
  while (fpu_depth < n) {
    emit2(0xDD, 0x81); /* fld qword [spill]     */
    emit_address(--spill_top);
    emit2(0xDD, 0xD9 + fpu_depth++); /* fstp st(depth + 1) */
  }
}

/* About to pop one value of a pair */
void fpu_pop(void) {
  fpu_need(2);
  fpu_depth--;
}

/*
 * Complex numbers live on the x87 stack as two values: the imaginary part
 * in st0 and the real part in st1.  Comments show the stack top first.
 */
void emit_load(double *address) {
  fpu_push();
  emit2(0xDD, 0x81); /* fld qword [address] */
  emit_address(address);
}

void emit_variable(int name) {
  emit_load(vars + name - 'a');
  emit_load(vars + name - 'a' + IMAG);
}

void emit_store(int name) {
  emit2(0xDD, 0x99); /* fstp qword [imag] */
  emit_address(vars + name - 'a' + IMAG);
  fpu_depth--;
  emit2(0xDD, 0x99); /* fstp qword [real] */
  emit_address(vars + name - 'a');
  fpu_depth--;
}

void emit_constant(void) {
  emit_load(next_constant++); /* real part */
  fpu_push();
  emit2(0xD9, 0xEE); /* fldz: imaginary part */
}

void emit_negate(void) {
  emit2(0xD9, 0xE0); /* fchs */
  emit2(0xD9, 0xC9); /* fxch st1 */
  emit2(0xD9, 0xE0); /* fchs */
  emit2(0xD9, 0xC9); /* fxch st1 */
}

void emit_real_part(void) /* im re -> 0 re */
{
  emit2(0xDD, 0xD8); /* fstp st0 */
  emit2(0xD9, 0xEE); /* fldz */
}

void emit_imag_part(void) /* im re -> 0 im */
{
  emit2(0xDD, 0xD9); /* fstp st1 */
  emit2(0xD9, 0xEE); /* fldz */
}

void emit_abs2(void) /* im re -> 0 re*re+im*im */
{
  emit2(0xD8, 0xC8); /* fmul st0, st0 */
  emit2(0xD9, 0xC9); /* fxch st1 */
  emit2(0xD8, 0xC8); /* fmul st0, st0 */
  emit2(0xDE, 0xC1); /* faddp st1 */
  emit2(0xD9, 0xEE); /* fldz */
}

void emit_exp(void) /* im re -> e^re*sin(im) e^re*cos(im) */
{
  fpu_push();
  emit2(0xD9, 0xFB); /* fsincos            cos sin re        */
  emit2(0xD9, 0xCA); /* fxch st2           re sin cos        */
  fpu_push();
  emit2(0xD9, 0xEA); /* fldl2e             log2(e) re ...    */
  emit2(0xDE, 0xC9); /* fmulp st1          t=re*log2(e)      */
  emit2(0xD9, 0xC0); /* fld st0            t t               */
  emit2(0xD9, 0xFC); /* frndint            n t               */
  emit2(0xDC, 0xE9); /* fsub st1, st0      n t-n             */
  emit2(0xD9, 0xC9); /* fxch st1           t-n n             */
  emit2(0xD9, 0xF0); /* f2xm1              2^(t-n)-1 n       */
  fpu_push();
  emit2(0xD9, 0xE8); /* fld1                                 */
  fpu_pop();
  emit2(0xDE, 0xC1); /* faddp st1          2^(t-n) n         */
  emit2(0xD9, 0xFD); /* fscale             e^re n sin cos    */
  fpu_pop();
  emit2(0xDD, 0xD9); /* fstp st1           e^re sin cos      */
  emit2(0xDC, 0xCA); /* fmul st2, st0      e^re sin cos*e^re */
  fpu_pop();
  emit2(0xDE, 0xC9); /* fmulp st1          sin*e^re cos*e^re */
}

void emit_ln(void) /* im re -> atan2(im,re) ln(re*re+im*im)/2 */
{
  fpu_push();
  emit2(0xD9, 0xE8); /* fld1               1 im re           */
  emit2(0xD9, 0xE0); /* fchs               -1 im re          */
  fpu_push();
  emit2(0xD9, 0xED); /* fldln2             ln2 -1 im re      */
  emit2(0xD9, 0xFD); /* fscale             ln2/2 -1 im re    */
  emit2(0xDD, 0xD9); /* fstp st1           ln2/2 im re       */
  emit2(0xD9, 0xC2); /* fld st2            re ln2/2 im re    */
  fpu_push();
  emit2(0xD9, 0xC2); /* fld st2            im re ln2/2 im re */
  emit2(0xD8, 0xC8); /* fmul st0, st0                        */
  emit2(0xD9, 0xC9); /* fxch st1                             */
  emit2(0xD8, 0xC8); /* fmul st0, st0                        */
  fpu_pop();
  emit2(0xDE, 0xC1); /* faddp st1          |z|^2 ln2/2 im re */
  fpu_pop();
  emit2(0xD9, 0xF1); /* fyl2x              ln|z| im re       */
  emit2(0xD9, 0xCA); /* fxch st2           re im ln|z|       */
  fpu_pop();
  emit2(0xD9, 0xF3); /* fpatan             arg ln|z|         */
}

/* Binary operators: stack is  b.im b.re a.im a.re */
void emit_add(void) {
  fpu_need(4);
  fpu_pop();
  emit2(0xDE, 0xC2); /* faddp st2: imaginary parts */
  fpu_pop();
  emit2(0xDE, 0xC2); /* faddp st2: real parts */
}

void emit_subtract(void) {
  fpu_need(4);
  fpu_pop();
  emit2(0xDE, 0xEA); /* fsubp st2: imaginary parts */
  fpu_pop();
  emit2(0xDE, 0xEA); /* fsubp st2: real parts */
}

void emit_multiply(void) {
  fpu_need(4);
  fpu_push();
  emit2(0xD9, 0xC3); /* fld st3            ar bi br ai ar       */
  emit2(0xD8, 0xCA); /* fmul st0, st2      ar*br bi br ai ar    */
  emit2(0xD9, 0xCC); /* fxch st4           ar bi br ai ar*br    */
  emit2(0xD8, 0xC9); /* fmul st0, st1      ar*bi bi br ai ar*br */
  emit2(0xD9, 0xCB); /* fxch st3           ai bi br ar*bi ..    */
  emit2(0xDC, 0xCA); /* fmul st2, st0      ai bi ai*br ..       */
  fpu_pop();
  emit2(0xDE, 0xC9); /* fmulp st1          ai*bi ai*br ar*bi ar*br */
  fpu_pop();
  emit2(0xDE, 0xEB); /* fsubp st3          ai*br ar*bi ar*br-ai*bi */
  fpu_pop();
  emit2(0xDE, 0xC1); /* faddp st1          ai*br+ar*bi ar*br-ai*bi */
}

void emit_divide(void) /* a / b = a * (conj(b) / |b|^2) */
{
  fpu_need(4);
  fpu_push();
  emit2(0xD9, 0xC1); /* fld st1            br bi br ...         */
  fpu_push();
  emit2(0xD9, 0xC1); /* fld st1            bi br bi br ...      */
  emit2(0xD8, 0xC8); /* fmul st0, st0                           */
  emit2(0xD9, 0xC9); /* fxch st1                                */
  emit2(0xD8, 0xC8); /* fmul st0, st0                           */
  emit2(0xDE, 0xC1); /* faddp st1          |b|^2 bi br ...      */
  emit2(0xD9, 0xE8); /* fld1               1 |b|^2 bi br ...    */
  fpu_pop();
  emit2(0xDE, 0xF1); /* fdivrp st1         1/|b|^2 bi br ...    */
  emit2(0xDC, 0xCA); /* fmul st2, st0                           */
  fpu_pop();
  emit2(0xDE, 0xC9); /* fmulp st1          bi/|b|^2 br/|b|^2 .. */
  emit2(0xD9, 0xE0); /* fchs               (1/b).im (1/b).re .. */
  emit_multiply();
}

/* Compare the real parts.  When the condition is false, return the number
 * of iterations left (held in edx). */
void emit_compare(int op) {
  fpu_need(4);
  emit2(0xDD, 0xD8); /* fstp st0: drop b.im                */
  emit2(0xDD, 0xD9); /* fstp st1: drop a.im   -> b.re a.re */
  if (sizeof(long) == 4) {
    emit2(0xDA, 0xE9); /* fucompp       */
    emit2(0xDF, 0xE0); /* fnstsw ax     */
    emit1(0x9E);       /* sahf          */
  } else {
    emit2(0xDF, 0xE9); /* fucomip st1   */
    emit2(0xDD, 0xD8); /* fstp st0      */
  }
  /* a < b is true when b.re is above a.re */
  emit2(op == OP_GREATER ? 0x72 : 0x77, 0x03); /* jb/ja over the return */
  emit2(0x89, 0xD0);                           /* mov eax, edx */
  emit1(0xC3);                                 /* ret */
  fpu_depth -= 4;
}

void emit_operator(int op) {
  switch (op) {
  case OP_NONE:
    break;
  case OP_ADD:
    emit_add();
    break;
  case OP_SUB:
    emit_subtract();
    break;
  case OP_MUL:
    emit_multiply();
    break;
  case OP_DIV:
  default:
    if (op <= OP_GREATER)
      emit_compare(op);
    else
      emit_divide();
    break;
  }
}

char *parse(char *s, int level);

/* Parse a number, variable, unary sign, function call or parentheses.
 * Returns a pointer just past it. */
char *parse_primary(char *s) {
  char *end;
  int ch = *s;

  *next_constant = strtod(s, &end);
  if (end != s) {
    emit_constant();
    return end;
  }
  if (ch < 6) { /* unary '+' or '-' */
    end = parse(s + 1, LEVEL_UNARY);
    if (ch == OP_SUB)
      emit_negate();
    return end;
  }
  if (ch > 'a' - 1) {
    emit_variable(ch);
    return s + 1;
  }
  /* Re( Im( Abs2( Exp( Ln( or just ( */
  end = parse(strchr(s, '(') + 1, LEVEL_COMPARE);
  switch (ch) {
  case 'R':
    emit_real_part();
    break;
  case 'I':
    emit_imag_part();
    break;
  case 'A':
    emit_abs2();
    break;
  case 'E':
    emit_exp();
    break;
  case 'L':
    emit_ln();
    break;
  }
  return strchr(end, ')') + 1;
}

/*
 * Parse operands joined by operators of the given precedence level.  Each
 * operand is parsed at the next higher level.  Returns a pointer to the
 * operator that ended the expression.
 */
char *parse(char *s, int level) {
  int op = OP_NONE;

  for (;;) {
    if (level == LEVEL_PRIMARY)
      s = parse_primary(s);
    else
      s = parse(s, level + 1);
    emit_operator(op);
    op = *s;
    if (op / 2 != level)
      return s;
    s++;
  }
}

/* Replace operators by their codes and remove all whitespace */
void tokenize(char *s) {
  static const char operators[] = ";<>+-*/";
  char *out = s;
  int k;

  do {
    for (k = 0; k < 7; k++)
      if (*s == operators[k]) {
        *s = k + 1;
        break;
      }
    *out = *s;
    if (!isspace(*out))
      out++;
  } while (*s++);
}

/*
 * Compile "init" (run once per pixel) and "loop" (run every iteration)
 * into a function returning the iterations left when a condition failed,
 * or 0 when every iteration passed.
 */
void compile(char **formulas) {
  char *loop_start = 0, *s;
  int f, name;

  emit1(0xE8); /* call next instruction */
  emit2(0, 0);
  emit2(0, 0);
  emit1(0x59); /* pop rcx: rcx = code_base + 5 */
  emit1(0xBA); /* mov edx, ITERATIONS */
  iterations = (Sint32 *)code;
  *iterations = ITERATIONS;
  code += 4;

  for (f = 0; f < 2; f++) {
    s = formulas[f];
    tokenize(s);
    loop_start = code;
    while ((name = *s)) {
      if (s[1] != '=')
        s = parse(s, LEVEL_COMPARE); /* condition */
      else {
        s = parse(s + 2, LEVEL_COMPARE); /* assignment */
        emit_store(name);
      }
      if (*s)
        s++; /* skip ';' */
    }
  }

  emit2(0xFF, 0xCA); /* dec edx */
  emit2(0x0F, 0x85); /* jnz loop_start */
  *(Sint32 *)code = loop_start - (code + 4);
  code += 4;
  emit2(0x89, 0xD0); /* mov eax, edx */
  emit1(0xC3);       /* ret */
}

int main(int argc, char **argv) {
  Sint32 (*fractal)(void);

  (void)argc;
  char *pixels;
  int pitch, cur = 0, buttons, zooming, mouse_x, mouse_y;
  int i, j, n, src_x, src_y, type, key;
  double aspect = pixel_step / pixel_step_x;
  float factor, inverse, dx, dy, dist;

  next_constant = mmap(0, MEMORY_SIZE, PROT_EXEC | PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANON, -1, 0);
  spill_top = next_constant + REGION_DOUBLES;
  vars = spill_top + REGION_DOUBLES;
  code = code_base = (char *)(vars + REGION_DOUBLES);
  compile(argv + 1);
  fractal = (Sint32(*)(void))code_base;

  SDL_Init(SDL_INIT_VIDEO);
  screen = SDL_SetVideoMode(WIDTH, HEIGHT, 8, 0);
  pixels = screen->pixels;
  pitch = screen->pitch;

  for (;;) {
    cur ^= 1;
    for (j = 0; j < HEIGHT; j++)
      for (i = 0; i < WIDTH; i++)
        samples[j][i][cur].dist = 9; /* no match yet */

    buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
    zooming = buttons & (SDL_BUTTON_LMASK | SDL_BUTTON_RMASK);
    if (zooming) {
      /* zoom by 2% towards the mouse (out with the right button) */
      factor = scale / 50;
      factor = buttons & SDL_BUTTON_LMASK ? factor : -factor;
      center_re += (mouse_x * pixel_step - aspect) * factor;
      center_im_neg += factor * mouse_y * pixel_step - factor;
      scale *= factor = 1 - factor / scale;
      inverse = 1 / factor;

      /* for each pixel find the nearest old sample */
      for (j = 0; j < HEIGHT; j++)
        for (i = 0; i < WIDTH; i++)
          for (n = 0; n < 4; n++) {
            src_x = mouse_x + (i - mouse_x) * factor + n / 2;
            src_y = mouse_y + (j - mouse_y) * factor + n % 2;
            if (src_x >= 0 && src_x < WIDTH && src_y >= 0 && src_y < HEIGHT) {
              struct sample *old = &samples[src_y][src_x][!cur];
              struct sample *new = &samples[j][i][cur];
              dx = mouse_x + (src_x + old->dx - mouse_x) * inverse - i;
              dy = mouse_y + (src_y + old->dy - mouse_y) * inverse - j;
              dist = dx * dx + dy * dy;
              if (dist < new->dist) {
                new->dx = dx;
                new->dy = dy;
                new->dist = dist;
                reused_color[j][i] = pixels[src_y * pitch + src_x];
              }
            }
          }
    }

    /* recalculate pixels that are a pixel or more from a sample */
    for (j = 0; j < HEIGHT; j++)
      for (i = 0; i < WIDTH; i++) {
        struct sample *s = &samples[j][i][cur];
        if ((int)s->dist) {
          vars['c' - 'a'] = (i * pixel_step - aspect) * scale + center_re;
          vars['c' - 'a' + IMAG] =
              scale - j * pixel_step * scale - center_im_neg;
          vars['i' - 'a'] = 0;
          vars['i' - 'a' + IMAG] = 1;
          n = fractal();
          pixels[j * pitch + i] = n ? *iterations - n : 0;
          s->dx = s->dy = s->dist = 0;
        } else
          pixels[j * pitch + i] = reused_color[j][i];
      }
    SDL_UpdateRect(screen, 0, 0, WIDTH, HEIGHT);

    while ((zooming ? SDL_PollEvent : SDL_WaitEvent)(&event)) {
      type = event.type;
      key = event.key.keysym.sym;
      if (type == SDL_QUIT || (type == SDL_KEYDOWN && key == SDLK_ESCAPE))
        return 0;
      if (type == SDL_MOUSEBUTTONDOWN)
        break;
      if (type == SDL_KEYDOWN &&
          (key == SDLK_KP_PLUS || key == SDLK_KP_MINUS)) {
        *iterations = key == SDLK_KP_MINUS ? *iterations / 2 : *iterations * 2;
        if (*iterations)
          break;
      }
    }
  }
}
