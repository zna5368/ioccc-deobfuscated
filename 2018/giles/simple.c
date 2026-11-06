#include "SDL.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_FILE "sand.txt"

/* the grid file is 49 rows of 81 cells plus a newline per row */
#define GRID_WIDTH  82
#define GRID_HEIGHT 49
#define GRID_SIZE   (GRID_WIDTH * GRID_HEIGHT)

/* cell values: file character minus '`' */
#define EMPTY 0 /* '`' */
#define SAND  1 /* 'a' */
#define WALL  2 /* 'b' (bit 1 marks a cell that never moves) */

#define CELL_SIZE 10
#define FRAME_DELAY_MS 40

/* print the failing line and SDL error then quit */
#define CHECK(cond)                                                       \
    if (!(cond)) {                                                        \
        printf("L%d SDL Error: %s\n", __LINE__, SDL_GetError());         \
        exit(1);                                                          \
    }

/*
 * Transition table for a 2x2 block of cells (Margolus neighbourhood).
 *
 * Input index bits:  1 = top-left is sand      2 = top-right is sand
 *                    4 = bottom-left occupied  8 = bottom-right occupied
 * Output bits use the same positions and say which cells hold sand/are
 * occupied after the step.
 *
 * The original stores this as the coefficients of a degree 15 polynomial
 * (hex encoded doubles) that maps each index to its output.
 */
static const int block_rule[16] = {
    0,  /*  0: nothing                               */
    4,  /*  1: top-left falls down                   */
    8,  /*  2: top-right falls down                  */
    12, /*  3: both fall down                        */
    4,  /*  4: nothing moves                         */
    12, /*  5: top-left slides down to the right     */
    12, /*  6: top-right falls down                  */
    13, /*  7: top-right falls, top-left stays       */
    8,  /*  8: nothing moves                         */
    12, /*  9: top-left falls down                   */
    12, /* 10: top-right slides down to the left     */
    14, /* 11: top-left falls, top-right stays       */
    12, /* 12: nothing moves                         */
    13, /* 13: blocked, stays                        */
    14, /* 14: blocked, stays                        */
    15, /* 15: full, stays                           */
};

/* toggles every step: which 2x2 block alignment is used */
int phase = 0;

/* apply the block rule to the 2x2 block whose top-left cell is at pos */
void update_block(int *grid, int width, int pos)
{
    int *top_left = &grid[pos];
    int *top_right = &grid[pos + 1];
    int *bottom_left = &grid[pos + width];
    int *bottom_right = &grid[pos + width + 1];

    int state = (*top_left == SAND)
              | 2 * (*top_right == SAND)
              | 4 * (*bottom_left != EMPTY)
              | 8 * (*bottom_right != EMPTY);
    int next = block_rule[state];

    /* keep the wall bit, replace the sand bit */
    *top_left = (*top_left & WALL) | (next & 1);
    *top_right = (*top_right & WALL) | (next >> 1 & 1);
    *bottom_left = (*bottom_left & WALL) | (next >> 2 & 1);
    *bottom_right = (*bottom_right & WALL) | (next >> 3 & 1);
}

/* one step of the cellular automaton, alternating block offsets */
void step(int *grid, int width, int height)
{
    int last_row = width * (height - 1);
    int row, col;

    phase ^= 1;
    for (row = phase ? width : 0; row < last_row; row += 2 * width)
        for (col = phase; col < width - 1; col += 2)
            update_block(grid, width, row + col);
}

int main(int argc, char **argv)
{
    int grid[GRID_SIZE];
    memset(grid, 0, GRID_SIZE * sizeof(int));

    FILE *file = fopen(argc > 1 ? argv[1] : DEFAULT_FILE, "r");
    if (file) {
        for (int i = 0; i < GRID_SIZE; i++)
            grid[i] = getc(file) - '`';
        fclose(file);
    }

    SDL_Window *window = 0;
    SDL_Renderer *renderer = 0;

    CHECK(SDL_Init(SDL_INIT_VIDEO) >= 0)
    atexit(SDL_Quit);
    CHECK(window = SDL_CreateWindow(DEFAULT_FILE, 0, 0, 800, 480,
                                    SDL_WINDOW_SHOWN))
    CHECK(renderer = SDL_CreateRenderer(window, -1, 0))

    int running = 1;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT)
                running = 0;
        }

        CHECK(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) >= 0);
        SDL_RenderClear(renderer);

        SDL_Rect rect;
        rect.w = rect.h = CELL_SIZE;

        step(grid, GRID_WIDTH, GRID_HEIGHT);

        /* pour a grain into the first empty spot of the top row on
         * every other step (phase is 1 on those steps, 0 otherwise) */
        int k = 0;
        while (k < GRID_WIDTH - 1 && grid[k])
            k++;
        grid[k] = phase;

        /* draw everything except the top row and the edge columns */
        for (int y = 1; y < GRID_HEIGHT; y++) {
            for (int x = 1; x < GRID_WIDTH - 1; x++) {
                rect.x = (x - 1) * CELL_SIZE;
                rect.y = (y - 1) * CELL_SIZE;
                int cell = grid[GRID_WIDTH * y + x];
                if (cell == SAND)
                    SDL_SetRenderDrawColor(renderer, 238, 204, 106, 255);
                else if (cell == EMPTY)
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                else
                    SDL_SetRenderDrawColor(renderer, 143, 149, 157, 255);
                SDL_RenderFillRect(renderer, &rect);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(FRAME_DELAY_MS);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return 0;
}
