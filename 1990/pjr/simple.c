#include <stdio.h>

/*
 * Every printing function writes a single character and then returns a
 * structure holding pointers to all of the printing functions.  Because the
 * return value has a member for every function, calls can be chained:
 *
 *     print_h().print_i().newline();
 */
typedef struct printers Printers;

struct printers {
    Printers (*print_a)(void);
    Printers (*print_b)(void);
    Printers (*print_c)(void);
    Printers (*print_d)(void);
    Printers (*print_e)(void);
    Printers (*print_f)(void);
    Printers (*print_g)(void);
    Printers (*print_h)(void);
    Printers (*print_i)(void);
    Printers (*print_j)(void);
    Printers (*print_k)(void);
    Printers (*print_l)(void);
    Printers (*print_m)(void);
    Printers (*print_n)(void);
    Printers (*print_o)(void);
    Printers (*print_p)(void);
    Printers (*print_q)(void);
    Printers (*print_r)(void);
    Printers (*print_s)(void);
    Printers (*print_t)(void);
    Printers (*print_u)(void);
    Printers (*print_v)(void);
    Printers (*print_w)(void);
    Printers (*print_x)(void);
    Printers (*print_y)(void);
    Printers (*print_z)(void);
    Printers (*space)(void);
    Printers (*newline)(void);
};

static Printers print_a(void);
static Printers print_b(void);
static Printers print_c(void);
static Printers print_d(void);
static Printers print_e(void);
static Printers print_f(void);
static Printers print_g(void);
static Printers print_h(void);
static Printers print_i(void);
static Printers print_j(void);
static Printers print_k(void);
static Printers print_l(void);
static Printers print_m(void);
static Printers print_n(void);
static Printers print_o(void);
static Printers print_p(void);
static Printers print_q(void);
static Printers print_r(void);
static Printers print_s(void);
static Printers print_t(void);
static Printers print_u(void);
static Printers print_v(void);
static Printers print_w(void);
static Printers print_x(void);
static Printers print_y(void);
static Printers print_z(void);
static Printers space(void);
static Printers newline(void);

static Printers printers = {
    print_a, print_b, print_c, print_d, print_e, print_f, print_g,
    print_h, print_i, print_j, print_k, print_l, print_m, print_n,
    print_o, print_p, print_q, print_r, print_s, print_t, print_u,
    print_v, print_w, print_x, print_y, print_z, space,   newline,
};

static Printers print_a(void) { printf("a"); return printers; }
static Printers print_b(void) { printf("b"); return printers; }
static Printers print_c(void) { printf("c"); return printers; }
static Printers print_d(void) { printf("d"); return printers; }
static Printers print_e(void) { printf("e"); return printers; }
static Printers print_f(void) { printf("f"); return printers; }
static Printers print_g(void) { printf("g"); return printers; }
static Printers print_h(void) { printf("h"); return printers; }
static Printers print_i(void) { printf("i"); return printers; }
static Printers print_j(void) { printf("j"); return printers; }
static Printers print_k(void) { printf("k"); return printers; }
static Printers print_l(void) { printf("l"); return printers; }
static Printers print_m(void) { printf("m"); return printers; }
static Printers print_n(void) { printf("n"); return printers; }
static Printers print_o(void) { printf("o"); return printers; }
static Printers print_p(void) { printf("p"); return printers; }
static Printers print_q(void) { printf("q"); return printers; }
static Printers print_r(void) { printf("r"); return printers; }
static Printers print_s(void) { printf("s"); return printers; }
static Printers print_t(void) { printf("t"); return printers; }
static Printers print_u(void) { printf("u"); return printers; }
static Printers print_v(void) { printf("v"); return printers; }
static Printers print_w(void) { printf("w"); return printers; }
static Printers print_x(void) { printf("x"); return printers; }
static Printers print_y(void) { printf("y"); return printers; }
static Printers print_z(void) { printf("z"); return printers; }
static Printers space(void) { printf(" "); return printers; }
static Printers newline(void) { printf("\n"); return printers; }

int main(void)
{
    /* "the quick brown fox jumped over the lazy dog" */
    print_t().print_h().print_e().space()
        .print_q().print_u().print_i().print_c().print_k().space()
        .print_b().print_r().print_o().print_w().print_n().space()
        .print_f().print_o().print_x().space()
        .print_j().print_u().print_m().print_p().print_e().print_d().space()
        .print_o().print_v().print_e().print_r().space()
        .print_t().print_h().print_e().space()
        .print_l().print_a().print_z().print_y().space()
        .print_d().print_o().print_g().newline();
    return 0;
}
