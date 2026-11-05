/*
 * The source of prog.c is one long line that is rebuilt from the template
 * below. Every fragment is followed by enough backspaces to move the terminal
 * cursor back to where the fragment started, so the next fragment overwrites
 * it and only the last text written to each column stays on the screen.
 */

/* Runs of backspaces, named by their length */
#define BACKSPACES_16 "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
#define BACKSPACES_18 BACKSPACES_16 "\b\b"
#define BACKSPACES_19 BACKSPACES_16 "\b\b\b"
#define BACKSPACES_20 BACKSPACES_16 "\b\b\b\b"
#define BACKSPACES_21 BACKSPACES_16 "\b\b\b\b\b"

/* An underscore and a letter typed twice over one column, which less shows as
 * an emphasized letter */
#define EMPHASIZE(letter) "_\b" letter "\b" letter

/* The template is printed inside a string literal, where a real quote or
 * backslash would end or change the string, so these stand in for them */
#define QUOTE_PLACEHOLDER '~'
#define BACKSLASH_PLACEHOLDER '#'

/* The backspaces before this index are left out of the decoded text */
#define FIRST_KEPT_BACKSPACE 115

/* The decoded text stops here, which leaves out the final run of backspaces */
#define DECODED_LENGTH 435

static const char template[] =
    "i"                     BACKSPACES_21
    "nt main(){ printf(~~)" BACKSPACES_21
    ";{char*p,s[999]=~%s"   BACKSPACES_19
    "~,*q=s,c;for(p=62**~"  BACKSPACES_20
    "~+q;q<s+435;c=q-s>*~s" BACKSPACES_21
    "~||c>8?*p++=c-35?*~}"  BACKSPACES_20
    "~<c?34:c:92:0,p[*~"    BACKSPACES_18
    "~]=0)c=*q++;*p=2+*~"   BACKSPACES_19
    "~;printf(s+62**~"      BACKSPACES_16
    "~,s);}return(*~#0"     BACKSPACES_16
    EMPHASIZE("H") EMPHASIZE("e") EMPHASIZE("l") EMPHASIZE("l") EMPHASIZE("o")
    EMPHASIZE(",") EMPHASIZE(" ")
    EMPHASIZE("w") EMPHASIZE("o") EMPHASIZE("r") EMPHASIZE("l") EMPHASIZE("d")
    EMPHASIZE("!")
    "#n~); }"               BACKSPACES_21;

/* Turn the template into a printf format that reproduces prog.c */
static void decode_template(char *decoded)
{
    int index;

    for (index = 0; index < DECODED_LENGTH; index++) {
        char letter = template[index];

        if (letter == '\b' && index < FIRST_KEPT_BACKSPACE) {
            continue;
        }

        if (letter == BACKSLASH_PLACEHOLDER) {
            letter = '\\';
        } else if (letter == QUOTE_PLACEHOLDER) {
            letter = '"';
        }

        *decoded++ = letter;
    }

    *decoded++ = '\n';
    *decoded = '\0';
}

int main()
{
    char format[sizeof template];

    decode_template(format);

    /* The %s in the format is where the template is printed word for word */
    printf(format, template);

    return 0;
}
