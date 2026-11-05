#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <dlfcn.h>
#include <time.h>

/*
 * The game compiles parts of itself while it runs. Each part is written to
 * t.c, built into a shared library ("plugin") and loaded with dlopen(). Every
 * plugin includes this file to get the declarations down to GENERATED_PLUGIN.
 *
 * A plugin cannot see the variables of the main program, so the address of
 * `game` is printed into the generated source as a plain number. The plugin's
 * constructor casts that number back to a pointer and stores its functions in
 * the structure, and that is how the main program gets to call them.
 */

typedef int (*Function)();

typedef struct {
	/* ncurses functions, filled in by the curses plugin */
	Function keypad;
	Function getmaxx;
	Function getmaxy;
	Function mvaddstr;
	Function refresh;
	Function cbreak;
	Function noecho;
	Function nonl;
	Function read_key;
	void *screen;

	/* filled in by the other plugins, the first time they are needed */
	Function handle_key;
	Function draw;

	int selected_action;
	int player_hp;
	int enemy_hp;
	int player_mp;
	int enemy_mp;
	int game_over;
} GameState;

/* read_key() returns these for the arrow keys, and any other key unchanged */
enum { MOVE_LEFT = -1, MOVE_RIGHT = -2 };

#ifndef GENERATED_PLUGIN

#define LORD_NAME "THE LEMONLORD"

/* the plugin libraries are named as in the original, which .gitignore expects */
#define CURSES_PLUGIN "static_assert"
#define DRAW_PLUGIN "volatile"
#define KEYS_PLUGIN "thread_local"

#define TURN_FINISHED 2

enum { ACTION_ATTACK, ACTION_REGENERATE, ACTION_HEAL };

/* backslashes are doubled because these rows end up in a generated string */
static const char *const screen_rows[] = {
	"; ; ; ; ; ; ; ; ;; ; ; ; ; ; ; ; ; ",
	"{                                } ",
	"{                                } ",
	"{                                } ",
	"{                                } ",
	"{  {HP} {MP}          {HP} {MP}  } ",
	"{                                } ",
	"{      O                  O      } ",
	"{     /|-}              {-|\\\\     } ",
	"{      |                  |      } ",
	"{     / \\\\                / \\\\     } ",
	"{                                } ",
	"{                                } ",
	"{                                } ",
	"; ; ; ; ; ; ; ; ;; ; ; ; ; ; ; ; ; ",
};

static const char *const curses_functions[] = {
	"keypad", "getmaxx", "getmaxy", "mvaddstr", "refresh",
	"read_key", "cbreak", "noecho", "nonl",
};

static char message[99] = " { { DEFEAT " LORD_NAME " } } ";

static GameState game = {
	.selected_action = ACTION_ATTACK,
	.player_hp = 30,
	.enemy_hp = 50,
	.player_mp = 5,
	.enemy_mp = 10,
};

/* a random number from 1 to sides */
static int roll(int sides)
{
	return 1 + rand() % sides;
}

static FILE *start_plugin(void)
{
	FILE *source = fopen("t.c", "w");

	fprintf(source, "#include \"simple.c\"\n");
	fprintf(source, "extern void plugin_init() __attribute__((constructor));\n");
	return source;
}

/* Compile t.c and load it, which runs its plugin_init() constructor. */
static void load_plugin(FILE *source, const char *library, const char *link_flags)
{
	char command[256];
	char path[64];

	fclose(source);
	sprintf(command, CC " -DGENERATED_PLUGIN -shared t.c -o %s %s 2>err",
		library, link_flags);
	if (system(command)) {
		exit(3);
	}
	sprintf(path, "./%s", library);
	dlopen(path, RTLD_LAZY);
}

static void write_menu_item(FILE *source, int row, int column, int action, const char *label)
{
	fprintf(source,
		"\tgame->mvaddstr(%d, %d, game->selected_action == %d ? \"{%s} \" : \" %s\");\n",
		row, column, action, label, label);
}

/* The screen position is worked out once and compiled into the drawing code. */
static void compile_draw_plugin(void)
{
	int left = (game.getmaxx(game.screen) - 32) / 2;
	int top = (game.getmaxy(game.screen) - 16) / 2;
	int rows = sizeof screen_rows / sizeof screen_rows[0];
	FILE *source = start_plugin();
	int row;

	fprintf(source, "int draw_game() {\n");
	fprintf(source, "\tGameState *game = (GameState *) %td;\n", (intptr_t) &game);
	fprintf(source, "\tchar *message = (char *) %td;\n", (intptr_t) message);
	fprintf(source, "\tchar stats[99];\n");
	fprintf(source, "\tint x = %d;\n", left);

	for (row = 0; row < rows; row++) {
		fprintf(source, "\tgame->mvaddstr(%d, x, \"%s\");\n", top + row, screen_rows[row]);
	}

	write_menu_item(source, top + 12, left + 3, ACTION_ATTACK, "ATTACK");
	write_menu_item(source, top + 12, left + 12, ACTION_REGENERATE, "REGENERATE");
	write_menu_item(source, top + 12, left + 25, ACTION_HEAL, "HEAL");

	fprintf(source, "\tgame->mvaddstr(%d, (int) (%d - strlen(message) / 2), message);\n",
		top + 2, left + 17);
	fprintf(source,
		"\tsprintf(stats, \"%%5d%%5d         %%5d%%5d\",\n"
		"\t\tgame->player_hp, game->player_mp, game->enemy_hp, game->enemy_mp);\n");
	fprintf(source, "\tgame->mvaddstr(%d, %d, stats);\n", top + 4, left + 1);
	fprintf(source, "\tgame->refresh();\n");
	fprintf(source, "\treturn 0;\n");
	fprintf(source, "}\n");

	fprintf(source, "void plugin_init() {\n");
	fprintf(source, "\t((GameState *) %td)->draw = draw_game;\n", (intptr_t) &game);
	fprintf(source, "}\n");

	load_plugin(source, DRAW_PLUGIN, "");
}

static void draw_screen(void)
{
	if (!game.draw) {
		compile_draw_plugin();
	}
	game.draw();
}

/* Redraw after an action, and announce the result if somebody has died. */
static void finish_action(void)
{
	const char *loser;
	const char *winner;

	draw_screen();

	game.game_over = game.player_hp * game.enemy_hp < 1;
	if (!game.game_over) {
		return;
	}

	loser = game.player_hp < 1 ? "You have" : LORD_NAME " has";
	winner = game.player_hp < 1 ? LORD_NAME " has" : "You have";

	if (sleep(1)) {
		return;
	}
	sprintf(message, "%s died!", loser);
	draw_screen();

	if (sleep(2)) {
		return;
	}
	sprintf(message, "%s won!", winner);
	draw_screen();

	sleep(3);
}

/* Returns 0 if the action could not be done, so the enemy does not get a turn. */
static int player_action(void)
{
	int amount;

	switch (game.selected_action) {
	case ACTION_ATTACK:
		amount = roll(16);
		game.enemy_hp -= amount;
		sprintf(message, "You hit for %d!", amount);
		break;
	case ACTION_REGENERATE:
		amount = roll(10);
		game.player_mp += amount;
		sprintf(message, "You gain %d MP!", amount);
		break;
	case ACTION_HEAL:
		if (game.player_mp <= 1) {
			sprintf(message, "You need more MP!");
			return 0;
		}
		game.player_mp -= roll(2);
		amount = roll(18);
		game.player_hp += amount;
		sprintf(message, "You heal %d!", amount);
		break;
	}
	return 1;
}

static int choose_enemy_action(void)
{
	if (game.player_hp < 9) {
		return ACTION_ATTACK;
	}

	if (game.enemy_hp < 20) {
		if (game.enemy_mp >= 2) {
			return ACTION_HEAL;
		}
		return game.player_mp >= 1 ? ACTION_REGENERATE : ACTION_ATTACK;
	}

	if (game.enemy_mp < 5 && roll(3) != 1 && game.player_mp >= 1) {
		return ACTION_REGENERATE;
	}

	if (roll(8) > 6 && (game.player_mp != 0 || game.enemy_mp > 1)) {
		if (game.enemy_mp > 9 || game.player_mp < 1) {
			return ACTION_HEAL;
		}
		return ACTION_REGENERATE;
	}

	return ACTION_ATTACK;
}

/* The enemy's "regenerate" steals MP from the player instead of creating it. */
static void enemy_action(void)
{
	int amount;

	switch (choose_enemy_action()) {
	case ACTION_ATTACK:
		amount = roll(12);
		game.player_hp -= amount;
		sprintf(message, "They hit for %d!", amount);
		break;
	case ACTION_REGENERATE:
		amount = roll(4);
		if (game.player_mp < amount) {
			amount = game.player_mp;
		}
		game.player_mp -= amount;
		game.enemy_mp += amount;
		sprintf(message, "They steal %d MP!", amount);
		break;
	case ACTION_HEAL:
		if (game.enemy_mp <= 1) {
			sprintf(message, "They need more MP!");
			break;
		}
		game.enemy_mp -= roll(2);
		amount = roll(24);
		game.enemy_hp += amount;
		sprintf(message, "They heal %d!", amount);
		break;
	}
}

static int play_turn(void)
{
	int action_done = player_action();

	finish_action();
	if (game.game_over || !action_done) {
		return TURN_FINISHED;
	}

	sleep(1);

	enemy_action();
	finish_action();
	return TURN_FINISHED;
}

static int quit_game(void)
{
	return game.game_over = 1;
}

/* The plugin holds a switch from each key to the address of its handler. */
static void compile_keys_plugin(void)
{
	static const struct {
		int key;
		Function handler;
	} bindings[] = {
		{ '\n', play_turn },
		{ '\r', play_turn },
		{ 27, quit_game },	/* Escape */
		{ ' ', play_turn },
		{ 'Q', quit_game },
		{ 'q', quit_game },
	};
	int count = sizeof bindings / sizeof bindings[0];
	FILE *source = start_plugin();
	int i;

	fprintf(source, "int handle_key(int key) {\n");
	fprintf(source, "\tswitch (key) {\n");
	for (i = 0; i < count; i++) {
		fprintf(source, "\tcase %d: return ((Function) %td)();\n",
			bindings[i].key, (intptr_t) bindings[i].handler);
	}
	fprintf(source, "\t}\n");
	fprintf(source, "\treturn 1;\n");
	fprintf(source, "}\n");

	fprintf(source, "void plugin_init() {\n");
	fprintf(source, "\t((GameState *) %td)->handle_key = handle_key;\n", (intptr_t) &game);
	fprintf(source, "}\n");

	load_plugin(source, KEYS_PLUGIN, "");
}

/* Only this plugin links ncurses, so the main program never mentions it. */
static void compile_curses_plugin(void)
{
	int count = sizeof curses_functions / sizeof curses_functions[0];
	FILE *source = start_plugin();
	int i;

	fprintf(source, "#include <ncurses.h>\n");

	fprintf(source, "void shut_down_curses() {\n");
	fprintf(source, "\tendwin();\n");
	fprintf(source, "\tputs(\"Play again?\");\n");
	fprintf(source, "}\n");

	fprintf(source, "int read_key() {\n");
	fprintf(source, "\tint key = getch();\n");
	fprintf(source, "\treturn key == KEY_LEFT ? MOVE_LEFT : key == KEY_RIGHT ? MOVE_RIGHT : key;\n");
	fprintf(source, "}\n");

	fprintf(source, "void plugin_init() {\n");
	fprintf(source, "\tGameState *game = (GameState *) %td;\n", (intptr_t) &game);
	for (i = 0; i < count; i++) {
		fprintf(source, "\tgame->%s = (Function) %s;\n",
			curses_functions[i], curses_functions[i]);
	}
	fprintf(source, "\tinitscr();\n");
	fprintf(source, "\tgame->screen = stdscr;\n");
	fprintf(source, "\tatexit(shut_down_curses);\n");
	fprintf(source, "}\n");

	load_plugin(source, CURSES_PLUGIN, "-lncurses");
}

int main(void)
{
	srand(time(0));

	compile_curses_plugin();
	game.cbreak();
	game.noecho();
	game.nonl();
	game.keypad(game.screen, 1);

	while (!game.game_over) {
		int key;

		draw_screen();
		key = game.read_key();

		if (key < 0) {
			/* MOVE_LEFT and MOVE_RIGHT step around the three menu items */
			game.selected_action = (game.selected_action + key + 3) % 3;
		} else {
			if (!game.handle_key) {
				compile_keys_plugin();
			}
			game.handle_key(key);
		}
	}
	return 0;
}

#endif /* GENERATED_PLUGIN */
