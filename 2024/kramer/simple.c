#include <stdio.h>

/*
 * English-language integer calculator.
 *
 * Each command line word is hashed to an 8-bit value which identifies it.
 * Number words are accumulated into a value, operator words are fed through
 * Dijkstra's shunting yard algorithm to produce a postfix (RPN) queue, and
 * the queue is then evaluated with a stack.
 */

#define TOKEN_SPACE 1000

/* operator codes: precedence is (code >> 2), so + - are 0 and * / % are 1 */
enum {
    OP_PLUS   = 1,
    OP_MINUS  = 2,
    OP_TIMES  = 5,
    OP_DIVIDE = 6,
    OP_MOD    = 7
};

/* word hashes as produced by hash_word() */
enum {
    H_SIX = 0,       H_EIGHT = 2,      H_TIMES = 16,     H_THREE = 17,
    H_TWENTY = 32,   H_THIRTY = 39,    H_FIVE = 40,      H_TWELVE = 45,
    H_FOUR = 47,     H_EIGHTY = 48,    H_FIFTY = 49,     H_FORTY = 52,
    H_DIVIDED = 55,  H_ELEVEN = 56,    H_SEVENTY = 65,   H_PLUS = 76,
    H_SIXTEEN = 78,  H_BILLION = 90,   H_SIXTY = 103,    H_SEVEN = 106,
    H_NEGATIVE = 110, H_NINETEEN = 111, H_TEN = 113,     H_TWO = 121,
    H_NINETY = 130,  H_MINUS = 137,    H_NINE = 165,     H_MILLION = 170,
    H_SEVENTEEN = 175, H_THOUSAND = 192, H_ONE = 201,    H_TRILLION = 202,
    H_THIRTEEN = 206, H_EIGHTEEN = 223, H_MOD = 232,     H_FOURTEEN = 238,
    H_FIFTEEN = 31,  H_HUNDRED = 242
    /* "zero", "and" and "by" are recognised only by doing nothing */
};

typedef struct {
    int is_number;      /* 1 = number, 0 = operator */
    long int value;     /* the number, or the operator code */
} token;

/*
 * One array holds two structures at once:
 *   - the output (postfix) queue grows upward from index 0
 *   - the operator / evaluation stack grows downward from TOKEN_SPACE
 */
token tokens[TOKEN_SPACE];
long int number;        /* value of the number built so far */
long int group;         /* current group below one thousand */
long int sign;          /* +1, or -1 after "negative" */
long int hash_state, length;
long int eval_pos;
long int queue_len;
long int stack_top = TOKEN_SPACE;

token make_token(long int value, int is_number)
{
    return (token){ .is_number = is_number, .value = value };
}

int hash_word(char *word)
{
    hash_state = 1;
    for (length = 0; word[length]; length++) {
        hash_state >>= 1;
        hash_state ^= word[length];
    }
    return (hash_state & 0xf) | ((length ^ word[0]) << 4 & 0xf0);
}

/* finish the current number and append it to the output queue */
void emit_number(void)
{
    number += group * sign;
    tokens[queue_len++] = make_token(number, 1);
    sign = 1;
    number = group = 0;
}

/* shunting yard: pop operators of higher or equal precedence, then push */
void push_operator(long int op)
{
    while (stack_top != TOKEN_SPACE && op >> 2 <= tokens[stack_top].value >> 2)
        tokens[queue_len++] = tokens[stack_top++];
    tokens[--stack_top] = make_token(op, 0);
}

void operator_word(long int op)
{
    emit_number();
    push_operator(op);
}

/* "thousand", "million", ... move the current group into the number */
void scale_word(long int scale)
{
    number += group * sign * scale;
    group = 0;
}

int main(int argc, char **argv)
{
    long int right, left, op, result;

    for (sign = 1; *++argv;) {
        switch (hash_word(*argv)) {
        case H_ONE:       group += 1;  break;
        case H_TWO:       group += 2;  break;
        case H_THREE:     group += 3;  break;
        case H_FOUR:      group += 4;  break;
        case H_FIVE:      group += 5;  break;
        case H_SIX:       group += 6;  break;
        case H_SEVEN:     group += 7;  break;
        case H_EIGHT:     group += 8;  break;
        case H_NINE:      group += 9;  break;
        case H_TEN:       group += 10; break;
        case H_ELEVEN:    group += 11; break;
        case H_TWELVE:    group += 12; break;
        case H_THIRTEEN:  group += 13; break;
        case H_FOURTEEN:  group += 14; break;
        case H_FIFTEEN:   group += 15; break;
        case H_SIXTEEN:   group += 16; break;
        case H_SEVENTEEN: group += 17; break;
        case H_EIGHTEEN:  group += 18; break;
        case H_NINETEEN:  group += 19; break;
        case H_TWENTY:    group += 20; break;
        case H_THIRTY:    group += 30; break;
        case H_FORTY:     group += 40; break;
        case H_FIFTY:     group += 50; break;
        case H_SIXTY:     group += 60; break;
        case H_SEVENTY:   group += 70; break;
        case H_EIGHTY:    group += 80; break;
        case H_NINETY:    group += 90; break;

        case H_HUNDRED:   group *= 100; break;
        case H_THOUSAND:  scale_word(1000L); break;
        case H_MILLION:   scale_word(1000L * 1000); break;
        case H_BILLION:   scale_word(1000L * 1000 * 1000); break;
        case H_TRILLION:  scale_word(1000L * 1000 * 1000 * 1000); break;

        case H_NEGATIVE:  sign = -1; break;

        case H_PLUS:      operator_word(OP_PLUS);   break;
        case H_MINUS:     operator_word(OP_MINUS);  break;
        case H_TIMES:     operator_word(OP_TIMES);  break;
        case H_DIVIDED:   operator_word(OP_DIVIDE); break;
        case H_MOD:       operator_word(OP_MOD);    break;
        }
    }
    emit_number();

    /* move all remaining operators to the output queue */
    while (stack_top != TOKEN_SPACE)
        tokens[queue_len++] = tokens[stack_top++];

    /* evaluate the postfix queue using the (now empty) stack */
    for (eval_pos = 0; eval_pos != queue_len; eval_pos++) {
        if (tokens[eval_pos].is_number) {
            tokens[--stack_top] = tokens[eval_pos];
            continue;
        }
        right = tokens[stack_top++].value;
        left = tokens[stack_top++].value;
        op = tokens[eval_pos].value;
        switch (op) {
        case OP_PLUS:   result = right + left; break;
        case OP_MINUS:  result = left - right; break;
        case OP_TIMES:  result = right * left; break;
        case OP_DIVIDE: result = left / right; break;
        default:        result = left % right; break;
        }
        tokens[--stack_top] = make_token(result, 1);
    }
    printf("%ld\n", tokens[stack_top].value);
    return 0;
}
