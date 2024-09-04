#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define CHAR_WIDTH2   4
#define CHAR_HEIGHT   6
#define TILE_SIZE     4
#define MOVE_SPEED    1
#define JUMP_VELOCITY -4
#define GRAVITY       0.5
#define FALL_GRAVITY  0.7
#define MAX_JUMPS     2

#define LEVEL_WIDTH  64
#define LEVEL_HEIGHT 16

typedef struct {
    int x;
    int y;
    float y_velocity;
    bool is_jumping;
    int jump_count;
} Character;

typedef enum {
    GameStatePlay,
    GameStatePause,
    GameStateWin,
    GameStateLose
} GameState;

typedef enum {
    MenuOptionContinue,
    MenuOptionRestart,
    MenuOptionExit,
    MenuOptionCount
} MenuOption;

typedef struct {
    Character character;
    bool left_pressed;
    bool right_pressed;
    InputKey last_direction;
    int camera_x;
    GameState state;
    uint32_t start_time;
    uint32_t pause_time;
    uint32_t total_pause_time;
    uint32_t win_time;
    MenuOption selected_menu_option;
} GameContext;

// 0: Empty, 1: Ground, 2: Platform, 3: Barrier, 4: Finish Line
const uint8_t level[LEVEL_HEIGHT][LEVEL_WIDTH] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};

static void draw_level(Canvas* canvas, int camera_x) {
    for(int y = 0; y < LEVEL_HEIGHT; y++) {
        for(int x = 0; x < LEVEL_WIDTH; x++) {
            int screen_x = x * TILE_SIZE - camera_x;
            int screen_y = y * TILE_SIZE;

            if(screen_x >= -TILE_SIZE && screen_x < SCREEN_WIDTH && screen_y >= -TILE_SIZE &&
               screen_y < SCREEN_HEIGHT) {
                switch(level[y][x]) {
                case 1: // Ground
                    canvas_draw_box(canvas, screen_x, screen_y, TILE_SIZE, TILE_SIZE);
                    break;
                case 2: // Platform
                    canvas_draw_box(canvas, screen_x, screen_y, TILE_SIZE, TILE_SIZE);
                    break;
                case 3: // Barrier
                    canvas_draw_box(canvas, screen_x, screen_y, TILE_SIZE, TILE_SIZE * 3);
                    break;
                case 4: // Finish Line
                    canvas_draw_line(
                        canvas, screen_x, screen_y, screen_x, screen_y + TILE_SIZE * LEVEL_HEIGHT);
                    break;
                }
            }
        }
    }
}

static void draw_character(Canvas* canvas, Character* character, int camera_x) {
    canvas_draw_box(canvas, character->x - camera_x, character->y, CHAR_WIDTH2, CHAR_HEIGHT);
}

static bool check_collision(int x, int y, int width, int height) {
    int tile_x1 = x / TILE_SIZE;
    int tile_x2 = (x + width - 1) / TILE_SIZE;
    int tile_y1 = y / TILE_SIZE;
    int tile_y2 = (y + height - 1) / TILE_SIZE;

    for(int tile_y = tile_y1; tile_y <= tile_y2; tile_y++) {
        for(int tile_x = tile_x1; tile_x <= tile_x2; tile_x++) {
            if(tile_x >= 0 && tile_x < LEVEL_WIDTH && tile_y >= 0 && tile_y < LEVEL_HEIGHT) {
                if(level[tile_y][tile_x] == 1 || level[tile_y][tile_x] == 2 ||
                   level[tile_y][tile_x] == 3) {
                    return true;
                }
            }
        }
    }
    return false;
}

static void update_character(Character* character, GameContext* game_context) {
    // Apply gravity
    character->y_velocity += (character->y_velocity > 0) ? FALL_GRAVITY : GRAVITY;

    // Update vertical position
    int new_y = character->y + (int)character->y_velocity;
    if(!check_collision(character->x, new_y, CHAR_WIDTH2, CHAR_HEIGHT)) {
        character->y = new_y;
        character->is_jumping = true;
    } else {
        // Find the ground
        while(check_collision(character->x, character->y + 1, CHAR_WIDTH2, CHAR_HEIGHT)) {
            character->y--;
        }
        character->y_velocity = 0;
        character->is_jumping = false;
        character->jump_count = 0;
    }

    // Check for win condition
    if(character->x >= (LEVEL_WIDTH - 1) * TILE_SIZE) {
        game_context->state = GameStateWin;
        game_context->win_time =
            (furi_get_tick() - game_context->start_time - game_context->total_pause_time) / 1000;
    }

    // Check for lose condition
    if(character->y >= LEVEL_HEIGHT * TILE_SIZE) {
        game_context->state = GameStateLose;
    }
}

static void draw_menu(
    Canvas* canvas,
    const char* title,
    const char* options[],
    int num_options,
    int selected_option) {
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, SCREEN_WIDTH / 2, 10, AlignCenter, AlignCenter, title);

    canvas_set_font(canvas, FontSecondary);
    int start_x = 40;
    int start_y = 30;
    int line_height = 15;

    for(int i = 0; i < num_options; i++) {
        if(i == selected_option) {
            canvas_draw_str(canvas, start_x - 10, start_y + i * line_height, ">");
        }
        canvas_draw_str(canvas, start_x, start_y + i * line_height, options[i]);
    }
}

static void flipper_platformer_game_draw_callback(Canvas* canvas, void* ctx) {
    const GameContext* game_context = ctx;
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    if(game_context->state == GameStatePlay) {
        draw_level(canvas, game_context->camera_x);
        draw_character(canvas, (Character*)&game_context->character, game_context->camera_x);

        // Draw timer
        char timer_str[20];
        uint32_t elapsed_time =
            (furi_get_tick() - game_context->start_time - game_context->total_pause_time) / 1000;
        snprintf(timer_str, sizeof(timer_str), "Time: %lu s", elapsed_time);
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 0, 10, timer_str);
    } else if(game_context->state == GameStatePause) {
        const char* pause_options[] = {"Continue", "Restart", "Exit"};
        draw_menu(canvas, "PAUSED", pause_options, 3, game_context->selected_menu_option);
    } else if(game_context->state == GameStateWin) {
        char win_title[50];
        snprintf(win_title, sizeof(win_title), "You Win! Time: %lu s", game_context->win_time);
        const char* win_options[] = {"Restart", "Exit"};
        draw_menu(canvas, win_title, win_options, 2, game_context->selected_menu_option);
    } else if(game_context->state == GameStateLose) {
        const char* lose_options[] = {"Restart", "Exit"};
        draw_menu(canvas, "Game Over", lose_options, 2, game_context->selected_menu_option);
    }
}

static void flipper_platformer_game_input_callback(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);
    FuriMessageQueue* event_queue = ctx;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

static void init_game(GameContext* game_context) {
    game_context->character.x = TILE_SIZE * 2;
    game_context->character.y = LEVEL_HEIGHT * TILE_SIZE - CHAR_HEIGHT - TILE_SIZE;
    game_context->character.y_velocity = 0;
    game_context->character.is_jumping = false;
    game_context->character.jump_count = 0;
    game_context->left_pressed = false;
    game_context->right_pressed = false;
    game_context->last_direction = InputKeyRight;
    game_context->camera_x = 0;
    game_context->state = GameStatePlay;
    game_context->start_time = furi_get_tick();
    game_context->pause_time = 0;
    game_context->total_pause_time = 0;
    game_context->win_time = 0;
    game_context->selected_menu_option = MenuOptionContinue;
}

int32_t flipper_platformer_game(void* p) {
    UNUSED(p);
    GameContext game_context;
    init_game(&game_context);

    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, flipper_platformer_game_draw_callback, &game_context);
    view_port_input_callback_set(view_port, flipper_platformer_game_input_callback, event_queue);

    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    InputEvent event;
    bool running = true;
    while(running) {
        if(furi_message_queue_get(event_queue, &event, 100) == FuriStatusOk) {
            if(event.type == InputTypePress) {
                switch(event.key) {
                case InputKeyUp:
                    if(game_context.state == GameStatePlay) {
                        if(game_context.character.jump_count < MAX_JUMPS) {
                            game_context.character.is_jumping = true;
                            game_context.character.y_velocity = JUMP_VELOCITY;
                            game_context.character.jump_count++;
                        }
                    } else if(
                        game_context.state == GameStatePause ||
                        game_context.state == GameStateWin ||
                        game_context.state == GameStateLose) {
                        if(game_context.selected_menu_option > 0) {
                            game_context.selected_menu_option--;
                        }
                    }
                    break;
                case InputKeyDown:
                    if(game_context.state == GameStatePause ||
                       game_context.state == GameStateWin || game_context.state == GameStateLose) {
                        int max_option = (game_context.state == GameStatePause) ?
                                             MenuOptionCount - 1 :
                                             MenuOptionExit;
                        if((int)game_context.selected_menu_option < max_option) {
                            game_context.selected_menu_option++;
                        }
                    }
                    break;
                case InputKeyLeft:
                    if(game_context.state == GameStatePlay) {
                        game_context.left_pressed = true;
                        game_context.last_direction = InputKeyLeft;
                    }
                    break;
                case InputKeyRight:
                    if(game_context.state == GameStatePlay) {
                        game_context.right_pressed = true;
                        game_context.last_direction = InputKeyRight;
                    }
                    break;
                case InputKeyOk:
                    if(game_context.state == GameStatePause) {
                        switch((int)game_context.selected_menu_option) {
                        case MenuOptionContinue:
                            game_context.state = GameStatePlay;
                            game_context.total_pause_time +=
                                furi_get_tick() - game_context.pause_time;
                            break;
                        case MenuOptionRestart:
                            init_game(&game_context);
                            break;
                        case MenuOptionExit:
                            running = false;
                            break;
                        }
                    } else if(
                        game_context.state == GameStateWin ||
                        game_context.state == GameStateLose) {
                        switch((int)game_context.selected_menu_option + 1) {
                        case MenuOptionRestart:
                            init_game(&game_context);
                            break;
                        case MenuOptionExit:
                            running = false;
                            break;
                        }
                    }
                    break;
                case InputKeyBack:
                    if(game_context.state == GameStatePlay) {
                        game_context.state = GameStatePause;
                        game_context.pause_time = furi_get_tick();
                        game_context.selected_menu_option = MenuOptionContinue;
                    } else {
                        running = false;
                    }
                    break;
                default:
                    break;
                }
            } else if(event.type == InputTypeRelease) {
                switch(event.key) {
                case InputKeyLeft:
                    game_context.left_pressed = false;
                    break;
                case InputKeyRight:
                    game_context.right_pressed = false;
                    break;
                default:
                    break;
                }
            }
        }

        if(game_context.state == GameStatePlay) {
            // Update character position based on button states
            int new_x = game_context.character.x;
            if(game_context.left_pressed && !game_context.right_pressed) {
                new_x -= MOVE_SPEED;
            } else if(game_context.right_pressed && !game_context.left_pressed) {
                new_x += MOVE_SPEED;
            } else if(game_context.left_pressed && game_context.right_pressed) {
                if(game_context.last_direction == InputKeyLeft) {
                    new_x -= MOVE_SPEED;
                } else {
                    new_x += MOVE_SPEED;
                }
            }

            // Check for collision and update if no collision
            if(!check_collision(new_x, game_context.character.y, CHAR_WIDTH2, CHAR_HEIGHT)) {
                game_context.character.x = new_x;
            }

            // Update camera position
            game_context.camera_x = game_context.character.x - SCREEN_WIDTH / 2;
            if(game_context.camera_x < 0) game_context.camera_x = 0;
            if(game_context.camera_x > LEVEL_WIDTH * TILE_SIZE - SCREEN_WIDTH) {
                game_context.camera_x = LEVEL_WIDTH * TILE_SIZE - SCREEN_WIDTH;
            }

            update_character(&game_context.character, &game_context);
        }

        view_port_update(view_port);
    }

    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);

    furi_record_close(RECORD_GUI);

    return 0;
}
