#include <stdio.h>
#include <string.h>

/*
 * The printable ASCII characters (' ' to '~'), listed in the order
 * of a QWERTY keyboard. A character's index in this table is its
 * value in the cipher.
 */
static const char alphabet[] =
    "1qaz2wsx3edc4rfv5tgb6yhn7ujm8ik,9ol.0p;/['`]\"\\ "
    "ZXCVBNM<>?~=:LKJHGFDSA!Q@W#E$R%T^Y&U*I(O)P_{+}|-";

int main(int argc, char *argv[])
{
    int size = strlen(alphabet); /* 95 printable characters */
    const char *key = alphabet + size; /* empty string: no key yet */
    const char *in_pos, *key_pos;
    int encrypt = 1;
    int previous = 0; /* index of the previous ciphertext character */
    int c, out;

    /* skip the program name */
    argv++;

    /* a leading "-" argument selects decryption */
    if (argc > 1 && strcmp(*argv, "-") == 0) {
        encrypt = 0;
        argv++;
        argc--;
    }

    while (1) {
        /* restart the key from its beginning when it runs out */
        if (*key == '\0' && argc > 1)
            key = *argv;

        c = fgetc(stdin);
        if (c == EOF)
            break;

        /* key character index (0 if there is no key or it is not printable) */
        key_pos = alphabet;
        if (*key != '\0') {
            key_pos = strchr(alphabet, *key);
            if (key_pos == NULL)
                key_pos = alphabet;
        }

        in_pos = strchr(alphabet, c);
        if (in_pos != NULL) {
            int index = in_pos - alphabet;
            int shift = previous - (key_pos - alphabet);

            /*
             * Encrypt adds (previous - key), decrypt subtracts it.
             * The 2 * size keeps the sum positive before taking modulo.
             */
            if (encrypt)
                out = (index + shift + 2 * size) % size;
            else
                out = (index - shift + 2 * size) % size;

            /* remember the ciphertext character for the next step */
            previous = encrypt ? out : index;
            c = alphabet[out];
        }
        fputc(c, stdout);

        /* advance to the next key character */
        if (*key != '\0')
            key++;
    }
    return 0;
}
