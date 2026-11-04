#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_PATCHES 8   /* number of bytes that may be changed */
#define CRC_DIGITS 8    /* length of the hex checksum string */

char *text;                               /* contents of the input file */
int text_size;
char candidates[MAX_PATCHES + 1][128];    /* per patch: chars to try */
int choice[MAX_PATCHES + 1];              /* current index into candidates */
int patch_pos[256];                       /* offsets of the patch bytes */
unsigned int partial_crc[MAX_PATCHES + 1];
unsigned int crc_table[256];
FILE *file;

/* Continue a CRC-32 register over text[start..end). */
unsigned int crc_update(unsigned int crc, int start, int end)
{
    while (start < end)
        crc = ((crc >> 8) & 0xffffff) ^ crc_table[(crc ^ text[start++]) & 255];
    return crc;
}

/* Print a message and use its negated length as the exit status. */
int message(char *msg)
{
    return -puts(msg);
}

int main(int argc, char **argv)
{
    int count[256];       /* character frequencies in the layout */
    int run_start[256];   /* start of the first run of 8 of each character */
    int marker, crc_pos, region_end, patches, run, prev, c, pos, j;
    unsigned int target;
    char *list;

    if (argc < 4)
        return message("<infile> <layout> <outfile>");

    /* Load the input file */
    text = 0;
    if ((file = fopen(argv[1], "rb")) != NULL && fseek(file, 0, SEEK_END) == 0) {
        text_size = ftell(file);
        if (text_size >= 8 && (text = (char *)malloc(text_size)) != NULL &&
            fseek(file, 0, SEEK_SET) == 0 && !fread(text, text_size, 1, file))
            ; /* read error: leave the file open */
        else
            fclose(file);
    }
    if (!text)
        return message("      bad input  ");

    /* Scan the layout file to find the marker character and patch spots */
    patches = 0;
    if ((file = fopen(argv[2], "rb")) != NULL) {
        for (c = 0; c < 256; c++) {
            count[c] = 0;
            run_start[c] = -1;
        }

        /* Count characters and find where each first repeats 8 times */
        prev = -1;
        run = 0;
        for (pos = 0; pos < text_size && (c = fgetc(file)) != EOF; pos++) {
            count[c]++;
            if (c == prev) {
                if (++run >= CRC_DIGITS && run_start[c] == -1)
                    run_start[c] = pos - (CRC_DIGITS - 1);
            } else {
                prev = c;
                run = 1;
            }
        }

        /* Marker: printable character with a run of 8 and lowest frequency */
        marker = -1;
        run = text_size + 1;
        for (c = '!'; c <= '~'; c++)
            if (run_start[c] != -1 && count[c] < run)
                run = count[marker = c];

        /* The first run of 8 markers holds the checksum; the next up to
           8 markers outside it are the bytes that may be changed */
        fseek(file, 0, SEEK_SET);
        if (marker != -1) {
            crc_pos = run_start[marker];
            for (pos = 0; patches < MAX_PATCHES && (c = fgetc(file)) != EOF &&
                          pos < text_size; pos++) {
                if ((pos < crc_pos || pos > crc_pos + CRC_DIGITS - 1) && c == marker) {
                    candidates[patches][0] = text[pos];
                    if (candidates[patches][0] == 0)
                        candidates[patches][0] = '.';
                    patch_pos[patches++] = pos;
                }
            }
        }
        fclose(file);
    }
    if (patches < 1)
        return message("  bad layout  ");

    /* Build the CRC-32 lookup table */
    for (j = 0; j < 256; j++) {
        unsigned int r = j;
        for (c = 8; c > 0; c--)
            r = (r & 1) == 0 ? r >> 1 : (r >> 1) ^ 0xedb88320;
        crc_table[j] = r;
    }

    /* Everything from region_end on is fixed: it covers all patch bytes and
       the checksum slot */
    region_end = patch_pos[patches - 1];
    if (region_end < crc_pos)
        region_end = crc_pos + CRC_DIGITS;
    else
        region_end++;
    patch_pos[patches] = region_end;

    /* Candidate list for each patch: original character first, then every
       printable character except '/', '\\' and the original */
    for (j = 0; j < patches; j++) {
        list = candidates[j] + 1;
        for (c = '!'; c <= '~'; c++)
            if (c != '/' && c != '\\' && c != candidates[j][0])
                *list++ = c;
        *list = 0;
        choice[j] = 0;
    }

    /* Pick a random CRC register value for region_end, work out the final
       checksum that results from it and write it into the checksum slot.
       The hex string is built inside the first candidate list, so that
       patch only gets to try the checksum digits. */
    target = time(0);
    sprintf(candidates[0] + 1, "%08x", crc_update(target, region_end, text_size) ^ ~0);
    for (j = 0; j < CRC_DIGITS; j++)
        text[crc_pos + j] = candidates[0][1 + j];

    /* Try every combination of patch characters (like an odometer) until
       the CRC register at region_end equals the chosen target */
    partial_crc[0] = crc_update(~0, 0, patch_pos[0]);
    j = 0;
    while (j >= 0) {
        /* Recompute only the partial CRCs after the changed patch */
        for (c = j; c < patches; c++) {
            text[patch_pos[c]] = candidates[c][choice[c]];
            partial_crc[c + 1] = crc_update(partial_crc[c], patch_pos[c], patch_pos[c + 1]);
        }

        if (partial_crc[patches] == target) {
            if ((file = fopen(argv[3], "wb+")) == NULL)
                return message("  \n  WriteError");
            if (!fwrite(text, text_size, 1, file))
                return 0;
            return fclose(file) | message("  \n    OK.");
        }

        /* Advance to the next combination, carrying into earlier patches */
        j = patches - 1;
        while (j >= 0 && !candidates[j][++choice[j]]) {
            for (c = j--; c < patches; c++)
                choice[c] = 0;
        }

        /* Show progress when one of the early patches changes */
        if (j < patches - 4) {
            putchar('.');
            fflush(stdout);
        }
    }
    return message("       \n                   fail");
}
