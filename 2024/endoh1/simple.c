#include <stdio.h>

/*
 * This program writes rt.c, a ray tracer made only of C preprocessor
 * directives.  Running "cc -E -P -DX=.. -DY=.. -DW=.. -DH=.. rt.c" renders the
 * red, green and blue values of a single pixel.
 *
 * Names of the macros inside rt.c (all one letter so rt.c stays small):
 *
 *   Scene:   p sphere center   q sphere radius   r sphere color
 *            s(x) floor color  t light position  u light color
 *            v(x) sky color
 *
 *   Math (fixed point, 1000 == 1.0):
 *            S scale           K(a,b) add        o(a,b) subtract
 *            k(a,b) multiply   U(a,b) divide     l(a) square
 *            n(a,b) one Newton square root step  m(x) square root
 *            T(x) checkerboard parity of a floor coordinate
 *
 *   Vectors are "x,y,z" tuples:
 *            A/B/C pick x/y/z  c(f,v) apply f to v  L(f,v) map f on v
 *            N(f,a,b) element-wise f(a,b)  O(f,a,s) f(element, scalar s)
 *            d(a,b) dot product
 *
 *   Ray:     P origin  Q direction  R distance to hit  h hit point
 *            g surface normal  D/E light reaching sphere/floor
 *            w color seen by the reflected (or floor/sky) ray  V final color
 */

/* highest bit copied by copy_value(); bit 0 is never copied */
#define TOP_BIT 30

static void define_macro(const char *name, const char *value)
{
    printf("#define %s %s\n", name, value);
}

/* #define name(a,b) body */
static void define_binary(const char *name, const char *body)
{
    printf("#define %s(a,b) %s\n", name, body);
}

static void begin_if(const char *condition)
{
    printf("#if %s\n", condition);
}

static void else_branch(void)
{
    puts("#else");
}

static void end_if(void)
{
    puts("#endif");
}

static void undefine(const char *name)
{
    printf("#undef %s\n", name);
}

/* recursion: the ray tracer includes itself to follow a reflected ray */
static void include_self(void)
{
    puts("#include __FILE__");
}

/*
 * Freeze the current value of macro `name`.  Every "#if" evaluates its
 * expression immediately, so each bit is tested and recorded in a chain of
 * helper macros name30 .. name0.  Afterwards `name` is redefined to the
 * resulting constant, so it no longer depends on any other macro.
 */
static void copy_value(const char *name)
{
    int bit;

    printf("#define %s%d 0\n", name, TOP_BIT);
    for (bit = TOP_BIT; bit > 0; bit--) {
        printf("#if (%s)/%d&1\n", name, 1 << bit);
        printf("#define %s%d %s%d|%d\n", name, bit - 1, name, bit, 1 << bit);
        else_branch();
        printf("#define %s%d %s%d\n", name, bit - 1, name, bit);
        end_if();
    }

    /* the bits hold the magnitude, so put the sign back */
    printf("#if %s>0\n", name);
    undefine(name);
    printf("#define %s (%s0)\n", name, name);
    else_branch();
    undefine(name);
    printf("#define %s -(%s0)\n", name, name);
    end_if();
}

/* forget a value made by copy_value() together with its helper macros */
static void undefine_copy(const char *name)
{
    int bit;

    undefine(name);
    for (bit = TOP_BIT; bit >= 0; bit--)
        printf("#undef %s%d\n", name, bit);
}

/* freeze each of the three components of vector macro `name` */
static void copy_vector(const char *name)
{
    const char *axis = "ABC";
    char component[16];
    int a;

    for (a = 0; a < 3; a++) {
        snprintf(component, sizeof component, "%s%c", name, axis[a]);
        printf("#define %s c(%c,%s)\n", component, axis[a], name);
        copy_value(component);
    }
    undefine(name);
    printf("#define %s %sA,%sB,%sC\n", name, name, name, name);
}

/*
 * Diffuse lighting at hit point h with normal g, stored in vector `light`:
 *   i = vector from h to the light
 *   M = |i|
 *   j = cosine between normal and light direction
 *   light = light color * j / |i|^2   (black when facing away)
 */
static void diffuse_light(const char *light)
{
    define_macro("i", "N(o,t,h)");
    define_macro("M", "m(d(i,i))");
    copy_value("M");
    define_macro("j", "d(g,O(U,i,M))");
    begin_if("j<0");
    define_macro(light, "0,0,0");
    else_branch();
    define_macro(light, "O(U,O(k,u,j),d(i,i))");
    end_if();
    copy_vector(light);
    undefine("i");
    undefine_copy("M");
    undefine("j");
}

/* the scene, editable by hand at the top of rt.c */
static void write_scene(void)
{
    define_macro("p", "0,500,4000");          /* sphere center */
    define_macro("q", "2000");                /* sphere radius */
    define_macro("r", "300,300,300");         /* sphere color */
    define_macro("s(x)", "900,200+x*700,900"); /* floor checker color */
    define_macro("t", "-2000,4000,1000");     /* light position */
    define_macro("u", "50000,50000,50000");   /* light color */
    define_macro("v(x)", "500-x,500-x,500");  /* sky color gradient */
}

/* trace the ray from origin P in direction Q; result goes to V */
static void write_tracer(void)
{
    /* sphere: solve |P + R*Q - p| = q for the distance R */
    define_macro("V", "N(o,P,p)");
    define_macro("G", "d(Q,Q)");
    define_macro("f", "d(V,Q)");
    define_macro("Z", "l(f)-k(G,(d(V,V)-l(q)))"); /* discriminant */
    begin_if("Z>0");
    define_macro("M", "m(Z)");
    copy_value("M");
    define_macro("R", "U(-M-f,G)");             /* nearer root */
    begin_if("R<0");
    undefine("R");
    define_macro("R", "U(M-f,G)");              /* farther root */
    begin_if("R<0");
    undefine("R");                              /* sphere is behind */
    end_if();
    end_if();
    end_if();

    /* sphere was hit: light it */
    begin_if("R");
    copy_value("R");
    undefine_copy("M");
    define_macro("h", "N(K,P,O(k,Q,R))");       /* hit point */
    define_macro("g", "O(U,N(o,h,p),q)");       /* normal */
    diffuse_light("D");
    end_if();

    undefine_copy("M");
    undefine("V");
    undefine("G");
    undefine("f");
    undefine("Z");

    begin_if("R>10");
    /* reflect the ray off the sphere and trace it again */
    define_macro("F", "N(K,O(k,g,10),h)");      /* new origin, nudged out */
    define_macro("I", "N(o,Q,O(k,g,d(g,Q)*2))"); /* reflected direction */
    copy_vector("F");
    copy_vector("I");
    undefine("P");
    undefine("Q");
    define_macro("P", "F");
    define_macro("Q", "I");
    undefine("h");
    undefine("g");
    undefine_copy("R");
    include_self();
    undefine("V");
    define_macro("V", "N(k,N(K,D,w),r)");       /* (diffuse + reflection) * color */
    else_branch();
    undefine("h");
    undefine("g");
    undefine_copy("R");

    /* floor: the plane y = -2 */
    begin_if("c(B,Q)");
    define_macro("R", "U(-2*S-c(B,P),c(B,Q))");
    begin_if("R<0||20000<R");
    undefine("R");                              /* behind or too far */
    end_if();
    end_if();

    begin_if("R");
    copy_value("R");
    define_macro("h", "N(K,P,O(k,Q,R))");
    define_macro("g", "0,S,0");                 /* floor normal points up */
    diffuse_light("E");
    end_if();

    begin_if("R>10");
    /* floor: light * (shadow under the sphere ? dark : 1) * checker color */
    define_macro("w", "N(k,O(k,E,l(c(A,h))+l((c(C,h)-c(C,p)))<l(q)?100:S),"
                      "s((T(c(A,h))==T(c(C,h)))))");
    else_branch();
    define_macro("w", "v(c(B,Q)/2)");           /* sky */
    end_if();
    copy_vector("w");
    define_macro("V", "w");
    end_if();
}

/* fixed point math and vector helpers used by the tracer */
static void write_library(void)
{
    define_macro("S", "1000");
    define_binary("K", "(a+b)");
    define_binary("o", "(a-(b))");
    define_binary("k", "((a)*(b)/S)");
    define_binary("U", "((a)*S/(b))");
    define_macro("l(a)", "k(a,a)");
    define_binary("n", "(b?(U(a,b)+b)/2:0)");
    define_macro("m(x)", "n(x,n(x,n(x,n(x,n(x,(x)/2)))))");
    define_macro("T(x)", "(x>0?x:S-x)/S%2");
    define_macro("A(x,y,z)", "x");
    define_macro("B(x,y,z)", "y");
    define_macro("C(x,y,z)", "z");
    define_binary("c", "a(b)");
    define_macro("L(a,b)", "a(A(b)),a(B(b)),a(C(b))");
    define_macro("N(x,a,b)", "x(A(a),A(b)),x(B(a),B(b)),x(C(a),C(b))");
    define_macro("O(x,a,b)", "x(A(a),(b)),x(B(a),(b)),x(C(a),(b))");
    define_binary("d", "(k(A(a),A(b))+k(B(a),B(b))+k(C(a),C(b)))");
}

/* emit one "#if" testing a decimal digit of color component `axis` */
static void begin_digit_if(char axis, int divisor, int modulus, int digit)
{
    printf("#if (c(%c,Z))/%d%%%d==%d\n", axis, divisor, modulus, digit);
}

/*
 * Choose a macro definition for each possible value of one decimal digit.
 * `format` is the definition with %d standing for the digit.
 */
static void define_per_digit(int divisor, char axis, const char *format)
{
    int digit;

    for (digit = 9; digit >= 0; digit--) {
        begin_digit_if(axis, divisor, 10, digit);
        printf("#define ");
        printf(format, digit);
        printf("\n");
        end_if();
    }
}

/*
 * Print color component `axis` as a plain decimal number.  The digits are
 * glued together with ## so that leading zeros are left out:
 *   x(a) adds the hundreds digit, y(a) adds the tens digit, z is the result.
 */
static void print_component(char axis)
{
    begin_digit_if(axis, 100, 10, 0);
    define_macro("x(a)", "a");
    else_branch();
    define_per_digit(100, axis, "x(a) %d##a");
    end_if();

    begin_digit_if(axis, 10, 100, 0);
    define_macro("y(a)", "a");
    else_branch();
    define_per_digit(10, axis, "y(a) x(%d##a)");
    end_if();

    define_per_digit(1, axis, "z y(%d)");
    puts("z");
    undefine("z");
    undefine("y");
    undefine("x");
}

int main(void)
{
    char axis;

    write_scene();

    /* S is only defined after the first pass, so "#if S" means "tracing" */
    begin_if("S");
    write_tracer();
    else_branch();

    /* first pass: set up, fire the primary ray, then print the result */
    write_library();
    define_macro("Q", "(S*X/W*2-S),(S-S*Y/H*2),S"); /* ray through pixel X,Y */
    define_macro("P", "0,0,0");                     /* camera at origin */
    include_self();
    define_macro("e(x)", "m(x<0?0:x>S?S:x)*255/S"); /* clamp, gamma, 0..255 */
    define_macro("Z", "L(e,V)");
    for (axis = 'A'; axis <= 'C'; axis++)
        print_component(axis);

    end_if();
    return 0;
}
