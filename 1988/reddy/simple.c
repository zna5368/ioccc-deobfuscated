/*
 * A tiny "cdecl": read C declarations such as
 *
 *     char *(*(fun[16]))();
 *
 * and explain them in English, one per input line.
 */
#include <stdio.h>
#include <ctype.h>

/*
 * Token kinds.  Punctuation is returned as the character itself, so the
 * two word-like kinds use small values that never appear as punctuation
 * (tabs and spaces are always skipped before a token is read).
 */
#define TOKEN_IDENT 9   /* name such as "char" or "fun" */
#define TOKEN_NUMBER 32 /* array size such as "16" */
#define TOKEN_ANY 0     /* advance without checking the current token */

char line[99];
char *cursor;     /* next unread character of the line */
char *token_text; /* start of the current token */
int token_len;    /* length of the current identifier or number */
int token;        /* kind of the current token */

/*
 * Move to the next token.  If `expected` is not TOKEN_ANY, only advance
 * when the current token is the expected one, otherwise stay put (a
 * missing ')' or ']' is silently ignored).
 */
int next_token(int expected)
{
    if (expected == TOKEN_ANY || expected == token) {
        while (*cursor == '\t' || *cursor == ' ')
            cursor++;

        token_text = cursor;
        token_len = 0;

        if (isalpha(*cursor))
            token = TOKEN_IDENT;
        else if (isdigit(*cursor))
            token = TOKEN_NUMBER;
        else
            token = *cursor++; /* punctuation, newline or '\0' */

        /* identifiers, numbers (and stray control characters) swallow
           the letters and digits that follow */
        if (token != 0 && token < 33)
            while (isalnum(*cursor)) {
                cursor++;
                token_len++;
            }
    }
    return token;
}

/*
 * Explain a declarator.  Prefix '*' binds looser than the suffixes
 * "()" and "[]", so the inner declarator is explained first, then the
 * suffixes, and the pointer last.
 */
void declarator(void)
{
    if (token == TOKEN_IDENT) {
        printf("`%.*s' is ", token_len, token_text);
        next_token(TOKEN_ANY);
    } else if (token == '(') {
        next_token(TOKEN_ANY);
        declarator();
        next_token(')');
    } else if (token == '*') {
        next_token(TOKEN_ANY);
        declarator();
        printf("ptr to ");
    }

    while (token == '(' || token == '[') {
        if (token == '(') {
            next_token(TOKEN_ANY);
            printf("func returning ");
            next_token(')');
        } else {
            if (next_token(TOKEN_ANY) == TOKEN_NUMBER) {
                printf("array[0..%d] of ", atoi(token_text) - 1);
                next_token(TOKEN_ANY);
            } else {
                printf("array of ");
            }
            next_token(']');
        }
    }
}

int main(void)
{
    char *type_name;
    int type_len;

    while (printf("input: "), (cursor = fgets(line, 99, stdin)) != NULL) {
        /* a declaration starts with its base type, e.g. "char" */
        if (next_token(TOKEN_ANY) == TOKEN_IDENT) {
            type_name = token_text;
            type_len = token_len;
            next_token(TOKEN_ANY);
            declarator();
            printf("%.*s.\n\n", type_len, type_name);
        }
    }
    return 0;
}
