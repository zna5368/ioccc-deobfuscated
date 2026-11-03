#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>
#include <math.h>

/*
 * A Reverse Polish (dc-like) calculator.  C embeds a Perl interpreter; the
 * Perl script below reads and parses the input, keeps the stack in the Perl
 * array @stack and handles printing.  The arithmetic operators are written in
 * C and registered as Perl subroutines (XSUBs) that work directly on @stack.
 */

/* Perl sees these C functions as subs defined in this file. */
#define SOURCE_FILE __FILE__

/*
 * Command dispatch: the Perl script takes the first character of a token,
 * computes (character % 39) and looks the result up in a table.  That means
 * every command has "synonyms": any other character with the same remainder
 * runs the same command (for example '=' is the same as 'd').
 */
#define COMMAND_MODULUS 39

typedef double (*binary_operation)(SV *left, SV *right);

/*
 * The divide-by-zero test looks at the current token ($token in Perl) the
 * same way the dispatcher does, so the '/' and '%' commands (and their
 * synonyms) are the only ones that refuse a zero right-hand operand.
 * The character is read as a C char, exactly like the original program.
 */
static int is_division_command(pTHX)
{
    char command = *SvPV(get_sv("token", 0), PL_na);
    int index = command % COMMAND_MODULUS;

    return index == '/' % COMMAND_MODULUS || index == '%' % COMMAND_MODULUS;
}

/*
 * Pop two values from @stack, apply the operation and push the result.
 * The value that was on top of the stack is the right-hand operand.
 */
static void apply_binary_operation(pTHX_ binary_operation operation)
{
    AV *stack = get_av("stack", 0);
    int division = is_division_command(aTHX);
    SV *right;
    SV *left;

    /* av_len() returns the highest index, so < 1 means fewer than 2 items */
    if (av_len(stack) < 1) {
        call_pv("stack_empty", 0);
        return;
    }

    right = av_pop(stack);
    left = av_pop(stack);

    if (division && SvNV(right) == 0) {
        /* put both operands back and complain */
        av_push(stack, left);
        av_push(stack, right);
        call_pv("divide_by_zero", 0);
        return;
    }

    av_push(stack, newSVnv(operation(left, right)));
}

static double add_values(SV *left, SV *right)
{
    return SvNV(right) + SvNV(left);
}

static double subtract_values(SV *left, SV *right)
{
    return SvNV(left) - SvNV(right);
}

static double multiply_values(SV *left, SV *right)
{
    return SvNV(right) * SvNV(left);
}

static double divide_values(SV *left, SV *right)
{
    return SvNV(left) / SvNV(right);
}

/* left - right * (left / right), where the division uses integers */
static double modulus_values(SV *left, SV *right)
{
    return SvNV(left) - (SvNV(right) * (SvIV(left) / SvIV(right)));
}

static double power_values(SV *left, SV *right)
{
    return pow(SvNV(left), SvNV(right));
}

XS(xs_add)      { apply_binary_operation(aTHX_ add_values); }
XS(xs_subtract) { apply_binary_operation(aTHX_ subtract_values); }
XS(xs_multiply) { apply_binary_operation(aTHX_ multiply_values); }
XS(xs_divide)   { apply_binary_operation(aTHX_ divide_values); }
XS(xs_modulus)  { apply_binary_operation(aTHX_ modulus_values); }
XS(xs_power)    { apply_binary_operation(aTHX_ power_values); }

/* Called by perl_parse() to make the C operators visible as Perl subs. */
static void xs_init(pTHX)
{
    newXSproto("add", xs_add, SOURCE_FILE, "");
    newXSproto("subtract", xs_subtract, SOURCE_FILE, "");
    newXSproto("multiply", xs_multiply, SOURCE_FILE, "");
    newXSproto("divide", xs_divide, SOURCE_FILE, "");
    newXSproto("modulus", xs_modulus, SOURCE_FILE, "");
    newXSproto("power", xs_power, SOURCE_FILE, "");
}

static const char calculator_script[] =
    "BEGIN { $| = 1 }  # flush output after every print\n"
    "\n"
    "our @stack;\n"
    "our $token;\n"
    "\n"
    "# (ord(first character) % 39) => command\n"
    "my %commands = (\n"
    "    ord('P') % 39 => 'pop_and_print',\n"
    "    ord('*') % 39 => 'multiply',\n"
    "    ord('+') % 39 => 'add',\n"
    "    ord('-') % 39 => 'subtract',\n"
    "    ord('/') % 39 => 'divide',\n"
    "    14            => 'stack_empty',  # e.g. '\\\\'\n"
    "    ord('^') % 39 => 'power',\n"
    "    ord('c') % 39 => 'clear_stack',\n"
    "    ord('d') % 39 => 'duplicate_top',\n"
    "    ord('f') % 39 => 'print_stack',\n"
    "    ord('p') % 39 => 'print_top',\n"
    "    ord('r') % 39 => 'swap_top_two',\n"
    "    ord('%') % 39 => 'modulus',\n"
    ");\n"
    "\n"
    "while (defined(my $line = <STDIN>)) {\n"
    "    foreach (split ' ', $line) {\n"
    "        $token = $_;\n"
    "        if ($token =~ /^poot$/i) {\n"
    "            print \"$token was here!\\n\";\n"
    "        } elsif ($token =~ /^\\s*(-?\\d+(\\.\\d+)?)\\s*$/) {\n"
    "            push @stack, $1;\n"
    "        } else {\n"
    "            my $command = $commands{ord($token) % 39};\n"
    "            if (defined $command) {\n"
    "                no strict 'refs';\n"
    "                &$command;\n"
    "            } else {\n"
    "                print \"unimplemented\\n\";\n"
    "            }\n"
    "        }\n"
    "    }\n"
    "}\n"
    "exit;\n"
    "\n"
    "sub stack_empty    { print \"stack empty\\n\" }\n"
    "sub divide_by_zero { print \"divide by zero\\n\" }\n"
    "\n"
    "sub clear_stack { @stack = () }\n"
    "\n"
    "sub print_top {\n"
    "    return stack_empty() if !@stack;\n"
    "    print $stack[-1] . \"\\n\";\n"
    "}\n"
    "\n"
    "sub pop_and_print {\n"
    "    return stack_empty() if !@stack;\n"
    "    print pop @stack;\n"
    "}\n"
    "\n"
    "sub print_stack {\n"
    "    print \"$_\\n\" foreach reverse @stack;\n"
    "}\n"
    "\n"
    "sub duplicate_top {\n"
    "    return stack_empty() if !@stack;\n"
    "    push @stack, $stack[-1];\n"
    "}\n"
    "\n"
    "# swap using arithmetic, so both values become numbers\n"
    "sub swap_top_two {\n"
    "    return stack_empty() if @stack < 2;\n"
    "    my $sum = $stack[-1] + $stack[-2];\n"
    "    $stack[-2] = $sum - $stack[-2];\n"
    "    $stack[-1] = $sum - $stack[-2];\n"
    "}\n";

int main(int argc, char **argv, char **env)
{
    PerlInterpreter *my_perl;
    char *perl_args[] = { "", "-e", "0" };

    PERL_SYS_INIT3(&argc, &argv, &env);
    my_perl = perl_alloc();
    perl_construct(my_perl);

    /* start an empty program ("perl -e 0") just to set up the interpreter */
    perl_parse(my_perl, xs_init, 3, perl_args, env);

    /* the real program; it calls exit itself at end of input */
    eval_sv(newSVpv(calculator_script, 0), 0);

    exit(3);
}
