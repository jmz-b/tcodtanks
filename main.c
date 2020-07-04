#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <libtcod.h>

#define CONSOLE_WIDTH 80
#define CONSOLE_HEIGHT 50
#define SCENARIO_WIDTH 80
#define SCENARIO_HEIGHT 50
#define SCENARIO_X 0
#define SCENARIO_Y 0
#define TORCH_RADIUS 10.0f
#define SQUARED_TORCH_RADIUS (TORCH_RADIUS*TORCH_RADIUS)
#define DARK_WALL (TCOD_color_t){0, 0, 100}
#define LIGHT_WALL (TCOD_color_t){130, 110, 50}
#define DARK_GROUND (TCOD_color_t){50, 50, 150}
#define LIGHT_GROUND (TCOD_color_t){200, 180, 50}

static const char smap1[SCENARIO_HEIGHT][SCENARIO_WIDTH]={
	{"################################################################################"},
	{"#                                                                              #"},
	{"#                                                                              #"},
	{"#                                                                              #"},
	{"#                                                                              #"},
	{"#                                                                              #"},
	{"#        ##########             ##############          #############          #"},
	{"#                                                                              #"},
	{"#                                                                              #"},
	{"#                                                                              #"},
	{"#                                                                              #"},
	{"#                                                                              #"},
	{"#                                                                              #"},
	{"#                                     #                                        #"},
	{"#                                     #                                        #"},
	{"#                                     #                                        #"},
	{"#                                     #                                        #"},
	{"#                                     #                                        #"},
	{"#                                     #                                        #"},
	{"#                                     #                                        #"},
	{"#                                                                              #"},
	{"#                                     #                                        #"},
	{"#                                                                              #"},
	{"#                                     #                                        #"},
	{"#                                                                              #"},
	{"#                                     #                                        #"},
	{"#                                                                              #"},
	{"#                                     #                                        #"},
	{"#                                                                              #"},
	{"#                                     #                                        #"},
	{"#                                     #                                        #"},
	{"#                                     #                                        #"},
	{"#                                     #                                        #"},
	{"#                                     #                                        #"},
	{"#                                     #                                        #"},
	{"#                                     #                                        #"},
	{"#                                     #                                        #"},
	{"#                                     #                                        #"},
	{"#                                                                              #"},
	{"#                                                                              #"},
	{"#                                                                              #"},
	{"#                                                                              #"},
	{"#                                                                              #"},
	{"#                                                                              #"},
	{"#                                                                              #"},
	{"#    ###################     #################         ################        #"},
	{"#                                                                              #"},
	{"#                                                                              #"},
	{"#                                                                              #"},
	{"################################################################################"},
};

typedef TCOD_console_t Console;
typedef TCOD_key_t Key;
typedef TCOD_mouse_t Mouse;
typedef TCOD_map_t Map;
typedef TCOD_renderer_t Renderer;

typedef struct {
	int x;
	int y;
} Cursor;

typedef struct {
	char *font;
	int nb_char_horiz;
	int nb_char_vertic;
	int font_flags;
	bool fullscreen;
	int fullscreen_width;
	int fullscreen_height;
	Renderer renderer;
} Config;

typedef struct {
	bool first;
	int recompute_fov;
} Scenario;

typedef struct {
	Cursor pos;	
} Unit;

typedef struct {
	char name[64];
	void (*run)(Scenario *scenario, Console *console, Key *key,  Mouse *mouse, Map *map, const char smap[SCENARIO_HEIGHT][SCENARIO_WIDTH], Unit *unit);
} Screen;

Config config_new(int argc,  char *argv[]) {
	char *font="/opt/libtcod/data/fonts/dejavu10x10_gs_tc.png";
	int nb_char_horiz=0, nb_char_vertic=0;
	int font_flags=TCOD_FONT_TYPE_GREYSCALE|TCOD_FONT_LAYOUT_TCOD;
	bool fullscreen=false;
	int fullscreen_width=0;
	int fullscreen_height=0;
	Renderer renderer=TCOD_RENDERER_SDL2;

	int font_new_flags=0;
	int argn;

	for (argn=1; argn < argc; argn++) {
		if (strcmp(argv[argn], "-font") == 0 && argn+1 < argc) {
			argn++;
			font=argv[argn];
			font_flags=0;
		} else if (strcmp(argv[argn], "-font-nb-char") == 0 && argn+2 < argc) {
			argn++;
			nb_char_horiz=atoi(argv[argn]);
			argn++;
			nb_char_vertic=atoi(argv[argn]);
			font_flags=0;
		} else if (strcmp(argv[argn], "-fullscreen-resolution") == 0 && argn+2 < argc) {
			argn++;
			fullscreen_width=atoi(argv[argn]);
			argn++;
			fullscreen_height=atoi(argv[argn]);
		} else if (strcmp(argv[argn], "-renderer") == 0 && argn+1 < argc) {
			argn++;
			renderer=(Renderer)atoi(argv[argn]);
		} else if (strcmp(argv[argn], "-fullscreen") == 0) {
			fullscreen=true;
		} else if (strcmp(argv[argn], "-font-in-row") == 0) {
			font_flags=0;
			font_new_flags |= TCOD_FONT_LAYOUT_ASCII_INROW;
		} else if (strcmp(argv[argn], "-font-greyscale") == 0) {
			font_flags=0;
			font_new_flags |= TCOD_FONT_TYPE_GREYSCALE;
		} else if (strcmp(argv[argn], "-font-tcod") == 0) {
			font_flags=0;
			font_new_flags |= TCOD_FONT_LAYOUT_TCOD;
		} else if (strcmp(argv[argn], "-help") == 0 || strcmp(argv[argn], "-?") == 0) {
			printf ("options :\n");
			printf ("-font <filename> : use a custom font\n");
			printf ("-font-nb-char <nb_char_horiz> <nb_char_vertic> : number of characters in the font\n");
			printf ("-font-in-row : the font layout is in row instead of columns\n");
			printf ("-font-tcod : the font uses TCOD layout instead of ASCII\n");
			printf ("-font-greyscale : antialiased font using greyscale bitmap\n");
			printf ("-fullscreen : start in fullscreen\n");
			printf ("-fullscreen-resolution <screen_width> <screen_height> : force fullscreen resolution\n");
			printf ("-renderer <num> : set renderer. 0 : GLSL 1 : OPENGL 2 : SDL\n");
			exit(0);
		} else {
			/* ignore parameter */
		}
	}

	if (font_flags == 0) font_flags=font_new_flags;

	return (Config) {font, nb_char_horiz, nb_char_vertic, font_flags, fullscreen, fullscreen_width, fullscreen_height, renderer};
}

Unit new_unit(int x, int y) {
	return (Unit){x, y};
}

void update_unit(Unit *unit, Key *key,  Mouse *mouse, Map *map) {
	if (key->vk == TCODK_TEXT && key->text[0] != '\0') {
		if (key->text[0] == 'K' || key->text[0] == 'k') {
			if (TCOD_map_is_walkable(*map, unit->pos.x, unit->pos.y-1)) {
				unit->pos.y--;
			}
		}
		else if (key->text[0] == 'J' || key->text[0] == 'j') {
			if (TCOD_map_is_walkable(*map, unit->pos.x, unit->pos.y+1)) {
				unit->pos.y++;
			}
		}
		else if (key->text[0] == 'H' || key->text[0] == 'h') {
			if (TCOD_map_is_walkable(*map, unit->pos.x-1, unit->pos.y)) {
				unit->pos.x--;
			}
		}
		else if (key->text[0] == 'L' || key->text[0] == 'l') {
			if (TCOD_map_is_walkable(*map, unit->pos.x+1, unit->pos.y)) {
				unit->pos.x++;
			}
		}
	}
}

void render_unit(Unit *unit, Console *console, Key *key,  Mouse *mouse) {
	if (key->vk == TCODK_TEXT && key->text[0] != '\0') {
		if (key->text[0] == 'K' || key->text[0] == 'k') {
			TCOD_console_put_char(*console,  unit->pos.x,  unit->pos.y+1,  ' ',  TCOD_BKGND_NONE);
			TCOD_console_put_char(*console,  unit->pos.x,  unit->pos.y,  '@',  TCOD_BKGND_NONE);
		}
		else if (key->text[0] == 'J' || key->text[0] == 'j') {
			TCOD_console_put_char(*console,  unit->pos.x,  unit->pos.y-1,  ' ',  TCOD_BKGND_NONE);
			TCOD_console_put_char(*console,  unit->pos.x,  unit->pos.y,  '@',  TCOD_BKGND_NONE);
		}
		else if (key->text[0] == 'H' || key->text[0] == 'h') {
			TCOD_console_put_char(*console,  unit->pos.x+1,  unit->pos.y,  ' ',  TCOD_BKGND_NONE);
			TCOD_console_put_char(*console,  unit->pos.x,  unit->pos.y,  '@',  TCOD_BKGND_NONE);
		}
		else if (key->text[0] == 'L' || key->text[0] == 'l') {
			TCOD_console_put_char(*console,  unit->pos.x-1,  unit->pos.y,  ' ',  TCOD_BKGND_NONE);
			TCOD_console_put_char(*console,  unit->pos.x,  unit->pos.y,  '@',  TCOD_BKGND_NONE);
		}
	}
}

Scenario new_scenario() {
	return (Scenario){true, true};
}

void render_scenario(Scenario *scenario, Console *console, Key *key,  Mouse *mouse, Map *map, const char smap[SCENARIO_HEIGHT][SCENARIO_WIDTH], Unit *unit) {
	bool is_visible = false;
	bool is_wall = false;

	if (scenario->first) {
		/* we draw the foreground only the first time.
		   during the player movement,  only the @ is redrawn.
		   the rest impacts only the background color */
		/* draw the help text & player @ */
		TCOD_console_clear(*console);
		TCOD_console_put_char(*console, unit->pos.x, unit->pos.y, '@', TCOD_BKGND_NONE);
		/* draw windows */
		for (int y=0; y < SCENARIO_HEIGHT; y++) {
			for (int x=0; x < SCENARIO_WIDTH; x++) {
				if (smap[y][x] == '=') {
					TCOD_console_put_char(*console, x, y, TCOD_CHAR_DHLINE, TCOD_BKGND_NONE);
				}
			}
		}
		scenario->first=false;
	}

	for (int y=0; y < SCENARIO_HEIGHT; y++) {
		for (int x=0; x < SCENARIO_WIDTH; x++) {
			is_visible = TCOD_map_is_in_fov(*map, x, y);
			is_wall=smap[y][x]=='#';

			if (!is_visible) {
				TCOD_console_set_char_background(*console, x, y,
					is_wall ? DARK_WALL:DARK_GROUND, TCOD_BKGND_SET);

			} else {
				// TCOD_console_set_char_background(*console, x, y,
				//	is_wall ? LIGHT_WALL : LIGHT_GROUND,  TCOD_BKGND_SET);
				TCOD_color_t base=(is_wall ? DARK_WALL : DARK_GROUND);
				TCOD_color_t light=(is_wall ? LIGHT_WALL : LIGHT_GROUND);
				float r=(x-unit->pos.x)*(x-unit->pos.x)+(y-unit->pos.y)*(y-unit->pos.y); /* cell distance to unit (squared) */
				if (r < SQUARED_TORCH_RADIUS) {
					float l = (SQUARED_TORCH_RADIUS-r)/SQUARED_TORCH_RADIUS;
					l=CLAMP(0.0f, 1.0f, l);
					base=TCOD_color_lerp(base, light, l);
				}
				TCOD_console_set_char_background(*console, x, y, base, TCOD_BKGND_SET);
			}
		}
	}
}


void run_scenario(Scenario *scenario, Console *console, Key *key,  Mouse *mouse, Map *map, const char smap[SCENARIO_HEIGHT][SCENARIO_WIDTH], Unit *unit) {
	if (key->vk == TCODK_TEXT && key->text[0] != '\0') {
		if (key->text[0] == 'K' || key->text[0] == 'k') {
			if (TCOD_map_is_walkable(*map, unit->pos.x, unit->pos.y-1)) {
				scenario->recompute_fov = true;
			}
		}
		else if (key->text[0] == 'J' || key->text[0] == 'j') {
			if (TCOD_map_is_walkable(*map, unit->pos.x, unit->pos.y+1)) {
				scenario->recompute_fov = true;
			}
		}
		else if (key->text[0] == 'H' || key->text[0] == 'h') {
			if (TCOD_map_is_walkable(*map, unit->pos.x-1, unit->pos.y)) {
				scenario->recompute_fov = true;
			}
		}
		else if (key->text[0] == 'L' || key->text[0] == 'l') {
			if (TCOD_map_is_walkable(*map, unit->pos.x+1, unit->pos.y)) {
				scenario->recompute_fov = true;
			}
		}
	}

	if (scenario->recompute_fov) {
		scenario->recompute_fov=false;
		TCOD_map_compute_fov(*map, unit->pos.x, unit->pos.y, (int)(TORCH_RADIUS), false,  FOV_RESTRICTIVE);
	}

	update_unit(unit, key, mouse, map);
	render_unit(unit, console, key, mouse);
	render_scenario(scenario, console, key, mouse, map, smap, unit);
}

int main(int argc,  char *argv[]) {
	Screen screens[] = {
		{"senario", run_scenario},
	};

	TCOD_Error error=TCOD_E_OK;
	Config config = config_new(argc, argv);
	Mouse mouse;
	Key key={TCODK_NONE, 0};
	Console screen_console=TCOD_console_new(SCENARIO_WIDTH, SCENARIO_HEIGHT);
	Map map=TCOD_map_new(SCENARIO_WIDTH, SCENARIO_HEIGHT);
	Scenario scenario=new_scenario();
	Unit unit=new_unit(20, 10);

	for (int y=0; y < SCENARIO_HEIGHT; y++) {
		for (int x=0; x < SCENARIO_WIDTH; x++) {
			if (smap1[y][x] == ' ') TCOD_map_set_properties(map, x, y, true, true); /* ground */
			else if (smap1[y][x] == '=') TCOD_map_set_properties(map, x, y, true, false); /* window */
		}
	}

	int cur_screen=0; /* index of the current screen */
	int nb_screens = sizeof(screens)/sizeof(Screen); /* total number of screens */

	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_WARN);

	TCOD_console_set_custom_font(config.font, config.font_flags, config.nb_char_horiz, config.nb_char_vertic);

	if (config.fullscreen_width > 0) {
		TCOD_sys_force_fullscreen_resolution(config.fullscreen_width, config.fullscreen_height);
	}

	atexit(TCOD_quit);

	error=TCOD_console_init_root(CONSOLE_WIDTH, CONSOLE_HEIGHT, "tcod tanks", config.fullscreen, config.renderer);	
	if (error) return error;
	
	TCOD_sys_set_fps(60); /* limited to 30 fps */

	do {
		/* update and render current screen */
		screens[cur_screen].run(&scenario, &screen_console, &key, &mouse, &map, smap1, &unit);

		/* blit the screen console on the root console */
		TCOD_console_blit(screen_console, 0, 0, SCENARIO_WIDTH, SCENARIO_HEIGHT,  /* the source console & zone to blit */
							NULL, SCENARIO_X, SCENARIO_Y,  /* the destination console & position */
							1.0f, 1.0f /* alpha coefs */
						);

		/* update the game screen */
		TCOD_console_flush();

		/* did the user hit a key ? */
		TCOD_sys_check_for_event((TCOD_event_t)(TCOD_EVENT_KEY_PRESS|TCOD_EVENT_MOUSE), &key, &mouse);
		
		if (key.vk == TCODK_ENTER && (key.lalt | key.ralt)) {
			/* ALT-ENTER : switch fullscreen */
			TCOD_console_set_fullscreen(!TCOD_console_is_fullscreen());
		}
	} while (!TCOD_console_is_window_closed());
	return 0;
}

