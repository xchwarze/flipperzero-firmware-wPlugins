#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>

#include <apple_grabber_icons.h>

#define MOVE        261.63f //C middle note from Piano
#define PROJECTILES 4 //Number of apples per round

// Screensize 128 p x 64 p

void draw_cb(Canvas* canvas, void* ap_pointer);
void input_cb(InputEvent* input, void* ap_pointer);
void timer_cb(void* ap_pointer);

enum FinalScore {
    LOSE,
    WIN,
    EXIT
};

typedef struct {
    int x;
    int y;
} Entity;

typedef struct {
    Entity coordinate;
    bool playing;
} Projectile;

typedef struct {
    int speed;
    int player_speed;
    int score;
    int catch;
    FuriMutex* model_mutex;
    FuriMessageQueue* event_queue;
    ViewPort* view_port;
    Gui* gui;
    FuriTimer* timer;
    Entity* player;
} AppleGame;

Projectile* apples[PROJECTILES];
bool playing = false;
int game_speed = 1;
enum FinalScore score = EXIT;

void free_apple_game(AppleGame* AP) {
    view_port_enabled_set(AP->view_port, false);
    gui_remove_view_port(AP->gui, AP->view_port);
    furi_record_close("gui");
    view_port_free(AP->view_port);
    furi_message_queue_free(AP->event_queue);
    furi_timer_free(AP->timer);

    furi_mutex_free(AP->model_mutex);

    if(furi_hal_speaker_is_mine()) {
        furi_hal_speaker_stop();
        furi_hal_speaker_release();
    }

    for(int i = 0; i < PROJECTILES; i++) {
        free(apples[i]);
    }

    free(AP->player);
    free(AP);
}

AppleGame* apple_allocation(int game_speed, int player_speed) {
    AppleGame* AP = malloc(sizeof(AppleGame));

    AP->model_mutex = furi_mutex_alloc(FuriMutexTypeNormal);

    AP->event_queue = furi_message_queue_alloc(1, sizeof(InputEvent));

    AP->view_port = view_port_alloc();
    view_port_draw_callback_set(AP->view_port, draw_cb, AP);
    view_port_input_callback_set(AP->view_port, input_cb, AP);

    AP->timer = furi_timer_alloc(timer_cb, FuriTimerTypePeriodic, AP);
    furi_timer_start(AP->timer, furi_kernel_get_tick_frequency() / 4);

    AP->gui = furi_record_open("gui");
    gui_add_view_port(AP->gui, AP->view_port, GuiLayerFullscreen);

    AP->speed = game_speed;
    AP->player_speed = player_speed;
    AP->score = 0;
    AP->catch = 0;

    AP->player = malloc(sizeof(Entity));

    AP->player->x = 64;
    AP->player->y = 49;

    for(int i = 0; i < PROJECTILES; i++) {
        apples[i] = malloc(sizeof(Projectile));
        apples[i]->coordinate.y = -rand() % 100;
        apples[i]->coordinate.x = rand() % 123;
        apples[i]->playing = true;
    }

    return AP;
}

void draw_cb(Canvas* canvas, void* ap_pointer) {
    AppleGame* AP = ap_pointer;
    furi_check(furi_mutex_acquire(AP->model_mutex, FuriWaitForever) == FuriStatusOk);

    Icon* player = (Icon*)&I_player; //Player Icon definition 10 x 15
    Icon* apple = (Icon*)&I_apple; //Apple Icon definition 5 x 6

    if(playing == false) {
        canvas_draw_frame(canvas, 0, 0, 128, 64);
        canvas_draw_str(canvas, 40, 10, "AppleGame");
        canvas_draw_str(canvas, 10, 20, "Press OK button to start");
        canvas_draw_str(canvas, 20, 30, "Use arrows to move");
    } else if(score != EXIT) {
        canvas_draw_frame(canvas, 0, 0, 128, 64);
        if(score == WIN) {
            canvas_draw_str(canvas, 10, 20, "You Won!!");
        } else {
            canvas_draw_str(canvas, 10, 20, "You Lost :(");
        }

    } else {
        canvas_draw_icon(canvas, AP->player->x, AP->player->y, player);
        for(int i = 0; i < PROJECTILES; i++) {
            if(apples[i]->playing == true) {
                canvas_draw_icon(canvas, apples[i]->coordinate.x, apples[i]->coordinate.y, apple);
            }
        }
    }

    furi_mutex_release(AP->model_mutex);
}

//For input callbacks
void input_cb(InputEvent* input, void* ap_pointer) {
    AppleGame* AP = ap_pointer;
    furi_message_queue_put(AP->event_queue, input, FuriWaitForever);
}

//For apple ticks
void timer_cb(void* ap_pointer) {
    AppleGame* AP = ap_pointer;
    InputEvent* input = malloc(sizeof(InputEvent));
    input->type = InputTypeMAX;
    furi_message_queue_put(AP->event_queue, input, FuriWaitForever);
}

int32_t apple_grabber_app(void* p) {
    UNUSED(p);

    InputEvent event;

    AppleGame* AP = apple_allocation(1, 1);
    bool running = true;

    //Playing phase
    while(running) {
        FuriStatus status = furi_message_queue_get(AP->event_queue, &event, FuriWaitForever);
        furi_check(furi_mutex_acquire(AP->model_mutex, FuriWaitForever) == FuriStatusOk);
        float volume = 1.0f;
        if(status == FuriStatusOk) {
            if(event.type == InputTypePress) {
                switch(event.key) {
                case InputKeyUp:
                    if(furi_hal_speaker_is_mine() || furi_hal_speaker_acquire(1000)) {
                        furi_hal_speaker_start(MOVE, volume);
                    }
                    break;
                case InputKeyDown:
                    if(furi_hal_speaker_is_mine() || furi_hal_speaker_acquire(1000)) {
                        furi_hal_speaker_start(MOVE, volume);
                    }
                    break;
                case InputKeyLeft:
                    AP->player->x -= 4 * game_speed;
                    if(furi_hal_speaker_is_mine() || furi_hal_speaker_acquire(1000)) {
                        furi_hal_speaker_start(MOVE, volume);
                    }
                    break;
                case InputKeyRight:
                    AP->player->x += 4 * game_speed;
                    if(furi_hal_speaker_is_mine() || furi_hal_speaker_acquire(1000)) {
                        furi_hal_speaker_start(MOVE, volume);
                    }
                    break;
                case InputKeyOk:
                    if(playing == false) {
                        playing = true;
                    }
                    break;
                case InputKeyBack:
                    running = false;
                    break;
                default:
                    break;
                }
            } else if(event.type == InputTypeRelease) {
                if(furi_hal_speaker_is_mine()) {
                    furi_hal_speaker_stop();
                    furi_hal_speaker_release();
                }
            } else if(event.type == InputTypeRepeat) {
                switch(event.key) {
                case InputKeyLeft:
                    AP->player->x -= 4 * game_speed;
                    if(furi_hal_speaker_is_mine() || furi_hal_speaker_acquire(1000)) {
                        furi_hal_speaker_start(MOVE, volume);
                    }
                    break;
                case InputKeyRight:
                    AP->player->x += 4 * game_speed;
                    if(furi_hal_speaker_is_mine() || furi_hal_speaker_acquire(1000)) {
                        furi_hal_speaker_start(MOVE, volume);
                    }
                    break;
                default:
                    break;
                }
            } else if(event.type == InputTypeMAX && playing) {
                bool apples_remaining = false;
                for(int i = 0; i < PROJECTILES; i++) {
                    if(apples[i]->playing) {
                        if(apples[i]->coordinate.y > 45 &&
                           apples[i]->coordinate.x >= AP->player->x &&
                           apples[i]->coordinate.x <= (AP->player->x + 10)) {
                            apples[i]->playing = false;
                        }
                        if(apples[i]->coordinate.y > 64) {
                            score = LOSE;
                            running = false;
                        } else {
                            apples_remaining = true;
                            apples[i]->coordinate.y += game_speed;
                        }
                    }
                }

                // If apples are caught, place them again
                if(apples_remaining == false) {
                    for(int i = 0; i < PROJECTILES; i++) {
                        apples[i]->coordinate.y = -40 - rand() % 80;
                        apples[i]->coordinate.x = rand() % 123;
                        apples[i]->playing = true;
                    }
                    game_speed += 1;
                }
            }
        }
        if(AP->player->x > 118) {
            AP->player->x = 118;
        } else if(AP->player->x < 0) {
            AP->player->x = 0;
        }
        if(game_speed == 4) {
            score = WIN;
            running = false;
        }

        furi_mutex_release(AP->model_mutex);
        view_port_update(AP->view_port);
    }

    //End result phase
    while(score != EXIT) {
        FuriStatus status = furi_message_queue_get(AP->event_queue, &event, FuriWaitForever);
        furi_check(furi_mutex_acquire(AP->model_mutex, FuriWaitForever) == FuriStatusOk);
        if(status == FuriStatusOk) {
            if(event.type == InputTypePress) {
                switch(event.key) {
                case InputKeyOk:
                    score = EXIT;
                    break;
                case InputKeyBack:
                    score = EXIT;
                    break;
                default:
                    break;
                }
            } else if(event.type == InputTypeRelease) {
                if(furi_hal_speaker_is_mine()) {
                    furi_hal_speaker_stop();
                    furi_hal_speaker_release();
                }
            }
        }
        furi_mutex_release(AP->model_mutex);
        view_port_update(AP->view_port);
    }

    free_apple_game(AP);

    return 0;
}
