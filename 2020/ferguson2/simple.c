#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/*
 * Enigma machine simulator (Heer / Luftwaffe three rotor model).
 *
 * Without arguments the machine uses the default settings: rotors I, II, III,
 * ring settings A A A, start positions A A A, reflector B and no plugboard.
 * With an argument that starts with '-' the settings are read from stdin
 * before the text (this is what recode produces).
 */

#define NUM_ROTORS     5
#define NUM_SLOTS      3
#define NUM_PLUGS     10

static const char ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/* wiring of rotors I to V; modified in place by the ring setting */
static char rotor_wiring[NUM_ROTORS][27] = {
    "EKMFLGDQVZNTOWYHXUSPAIBRCJ",       /* I   */
    "AJDKSIRUXBLHWTMCQGZNPYFVOE",       /* II  */
    "BDFHJLCPRTXVZNYEIWGAKMUSQO",       /* III */
    "ESOVPZJAYQUIRHXLNFTGKDCMWB",       /* IV  */
    "VZBRGITYUPSDNHLXAWMJQOFECK",       /* V   */
};

/* turnover notch of rotors I to V */
static const char rotor_notch[NUM_ROTORS] = { 'Q', 'E', 'V', 'J', 'Z' };

/* reflectors B and C */
static const char reflector_wiring[2][27] = {
    "YRUHQSLDPXNGOKMIEBFZCWVJAT",       /* B */
    "FVPJIAOYEDRZXWGCTKUQSBNMHL",       /* C */
};

/* machine settings: slot 0 is the left rotor, slot 2 the right rotor */
static int  slot_rotor[NUM_SLOTS]    = { 0, 1, 2 };
static char ring_setting[NUM_SLOTS]  = { 'A', 'B', 'C' };
static char start_position[NUM_SLOTS] = { 'B', 'B', 'C' };
static int  reflector = 0;
static char plug_from[NUM_PLUGS + 1];
static char plug_to[NUM_PLUGS + 1];

/* derived by setup_rotors() */
static char *slot_wiring[NUM_SLOTS];
static char slot_notch[NUM_SLOTS];

static int letter_index(char c)
{
    return strchr(ALPHABET, c) - ALPHABET;
}

static char next_letter(char c)
{
    return ALPHABET[(letter_index(c) + 1) % 26];
}

/* swap a letter through the plugboard */
static char plugboard(char k)
{
    int i;

    for (i = 0; i < NUM_PLUGS; ++i) {
        if (plug_to[i] && plug_from[i] == k)
            return plug_to[i];
        if (plug_from[i] && plug_to[i] == k)
            return plug_from[i];
    }
    return k;
}

/* Caesar shift every capital letter of a rotor wiring */
static void shift_wiring(char *wiring, int shift)
{
    int i;

    for (i = 0; wiring[i]; ++i) {
        int c = wiring[i];
        if (c >= 'A' && c <= 'Z')
            wiring[i] = (c - 'A' + shift) % 26 + 'A';
    }
}

/*
 * Put the selected rotors into the slots. The ring setting is applied by
 * shifting the wiring, but only the ring setting of the first slot is used
 * (for all three rotors).
 */
static void setup_rotors(void)
{
    int slot;

    for (slot = 0; slot < NUM_SLOTS; ++slot) {
        slot_notch[slot] = rotor_notch[slot_rotor[slot]];
        slot_wiring[slot] = rotor_wiring[slot_rotor[slot]];
        shift_wiring(slot_wiring[slot], letter_index(ring_setting[0]));
    }
}

/* right to left through a rotor at the given offset */
static char rotor_forward(const char *wiring, int offset, char k)
{
    char v = wiring[(letter_index(k) + offset) % 26];
    return ALPHABET[(letter_index(v) - offset + 26) % 26];
}

/* left to right through a rotor at the given offset */
static char rotor_backward(const char *wiring, int offset, char k)
{
    char v = ALPHABET[(letter_index(k) + offset) % 26];
    int pos = strchr(wiring, v) - wiring;
    return ALPHABET[(pos - offset + 26) % 26];
}

/* encipher (or decipher) a string in place */
static void encipher(char *text)
{
    char *p;
    char left = start_position[0];
    char middle = start_position[1];
    char right = start_position[2];

    for (p = text; *p; ++p) {
        int was_lower = islower(*p);

        if (isalpha(*p = toupper(*p))) {
            char k = *p;
            int left_off, middle_off, right_off;
            int right_at_notch = (right == slot_notch[2]);

            /* step the rotors; the right rotor always steps */
            right = next_letter(right);
            if (right_at_notch) {
                /* the left rotor is never stepped here */
                middle = next_letter(middle);
            } else if (middle == slot_notch[1]) {
                middle = next_letter(middle);
                left = next_letter(left);
            }

            k = plugboard(k);

            left_off = letter_index(left);
            middle_off = letter_index(middle);
            right_off = letter_index(right);

            k = rotor_forward(slot_wiring[2], right_off, k);
            k = rotor_forward(slot_wiring[1], middle_off, k);
            k = rotor_forward(slot_wiring[0], left_off, k);

            if (reflector_wiring[reflector][k - 'A'])
                k = reflector_wiring[reflector][k - 'A'];

            k = rotor_backward(slot_wiring[0], left_off, k);
            k = rotor_backward(slot_wiring[1], middle_off, k);
            k = rotor_backward(slot_wiring[2], right_off, k);

            k = plugboard(k);

            *p = was_lower ? tolower(k) : k;
        }
    }
}

/* store a letter setting if the input is a letter */
static void set_letter(char *setting, int c)
{
    if (isalpha(c = toupper(c)))
        *setting = c;
}

static int plug_used(char c)
{
    return strchr(plug_from, c) || strchr(plug_to, c);
}

/* read one character, skipping newlines */
static int read_char(void)
{
    int c;

    do
        c = getchar();
    while (c == '\n');
    return c;
}

static void read_settings(void)
{
    int i, c;

    for (i = 0; i < NUM_SLOTS; ++i) {
        fprintf(stderr, "%s %d: ", "Ring", i + 1);
        c = read_char() - '0';
        slot_rotor[i] = (c < 1 ? 1 : (c > 5 ? 5 : c)) - 1;

        fprintf(stderr, "%s %d: ", "Setting", i + 1);
        set_letter(&ring_setting[i], read_char());

        fprintf(stderr, "%s %d: ", "Position", i + 1);
        set_letter(&start_position[i], read_char());
    }

    /* any character greater than '\1' selects reflector C */
    fprintf(stderr, "%s: ", "Reflector");
    c = read_char();
    reflector = (c > 1) ? 1 : 0;

    for (i = 0; i < NUM_PLUGS; ++i) {
        int a, b;

        fprintf(stderr, "%s %d: ", "Plugboard pair", i + 1);
        a = read_char();
        b = read_char();
        if (isalpha(a = toupper(a)) && isalpha(b = toupper(b)) && a != b &&
            !plug_used(b) && !plug_used(a)) {
            plug_from[i] = a;
            plug_to[i] = b;
        }
    }
}

int main(int argc, char **argv)
{
    char *text = NULL;
    size_t size = 0;

    (void)argc;

    if (argv[1] && *argv[1] == '-')
        read_settings();

    setup_rotors();
    if (getdelim(&text, &size, EOF, stdin) > 0) {
        encipher(text);
        printf("%s", text);
    }
    free(text);
    return 0;
}
