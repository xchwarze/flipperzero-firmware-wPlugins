#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>
#include <storage/storage.h>

#define MAX_COUNT      4
#define OFFSET_Y       5
#define SAVE_FILE_PATH EXT_PATH("lishi_values.txt")

#define MAX_KEYS      6
#define TOY48_COLUMNS 5
#define OTHER_COLUMNS 8

typedef enum {
    MenuMain,
    MenuConfig,
    MenuShow,
    MenuAbout,
} MenuState;

typedef struct {
    FuriMessageQueue* input_queue;
    ViewPort* view_port;
    Gui* gui;
    FuriMutex* mutex;
    NotificationApp* notifications;

    int counts[MAX_KEYS][OTHER_COLUMNS];
    bool pressed[8];
    int boxtimer[8];
    int active_column;
    int selected_menu;
    MenuState menu_state;
    int selected_key;
    char current_key[10];
} Lishi;

void state_free(Lishi* l) {
    if(l->view_port) gui_remove_view_port(l->gui, l->view_port);
    if(l->gui) furi_record_close(RECORD_GUI);
    if(l->view_port) view_port_free(l->view_port);
    if(l->input_queue) furi_message_queue_free(l->input_queue);
    if(l->mutex) furi_mutex_free(l->mutex);
    free(l);
}

static void input_callback(InputEvent* input_event, void* ctx) {
    Lishi* l = ctx;
    if(input_event->type == InputTypeShort || input_event->type == InputTypeLong) {
        furi_message_queue_put(l->input_queue, input_event, 0);
    }
}

static void render_callback(Canvas* canvas, void* ctx) {
    Lishi* l = ctx;
    furi_check(furi_mutex_acquire(l->mutex, FuriWaitForever) == FuriStatusOk);
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    if(l->menu_state == MenuMain) {
        canvas_draw_str(canvas, 5, 10, "LISHI Main Menu");
        const char* options[] = {"Config", "Show", "About"};
        for(int i = 0; i < 3; i++) {
            if(i == l->selected_menu) {
                canvas_draw_str(canvas, 5, 30 + i * 15, ">");
            }
            canvas_draw_str(canvas, 15, 30 + i * 15, options[i]);
        }
    } else if(l->menu_state == MenuConfig) {
        canvas_draw_str(canvas, 5, 10, "Select Key Type");

        const char* keys[MAX_KEYS] = {"HU66", "HU92", "HU83", "K5", "TOY48", "FO38"};
        for(int i = 0; i < MAX_KEYS; i++) {
            int y_pos = 30 + (i % 3) * 15;
            int x_pos = 15 + (i / 3) * 40;

            if(i == l->selected_key) {
                canvas_draw_str(canvas, x_pos - 10, y_pos, ">");
            }
            canvas_draw_str(canvas, x_pos, y_pos, keys[i]);
        }
    } else if(l->menu_state == MenuShow) {
        char lishi_key[20];
        snprintf(lishi_key, sizeof(lishi_key), "LISHI %s", *l->current_key ? l->current_key : "");
        canvas_draw_str(canvas, 5, 10, *l->current_key ? lishi_key : "LISHI Key View");

        int* current_counts = l->counts[l->selected_key];

        canvas_set_font(canvas, FontSecondary);
        for(int i = 0; i < (l->selected_key == 4 ? TOY48_COLUMNS : OTHER_COLUMNS); i++) {
            char dynamic_number[2];
            snprintf(dynamic_number, sizeof(dynamic_number), "%d", i + 1);
            canvas_draw_str_aligned(
                canvas, 10 + (i * 15), 35, AlignCenter, AlignCenter, dynamic_number);
        }
        for(int i = 0; i < (l->selected_key == 4 ? TOY48_COLUMNS : OTHER_COLUMNS); i++) {
            char scount[8];
            snprintf(scount, sizeof(scount), "%d", current_counts[i]);
            canvas_draw_str_aligned(canvas, 10 + (i * 15), 50, AlignCenter, AlignCenter, scount);
        }

        size_t active_x = 10 + (l->selected_key == 4 ? l->active_column : l->active_column) * 15;
        canvas_draw_rframe(canvas, active_x - 6, 40, 12, 20, 2);
    } else if(l->menu_state == MenuAbout) {
        canvas_draw_str(canvas, 5, 10, "About LISHI App");
        canvas_draw_str(canvas, 10, 30, "Version 0.2");
        canvas_draw_str(canvas, 10, 45, "Author: evillero");
        canvas_draw_str(canvas, 10, 60, "www.github.com/evillero");
    }

    furi_mutex_release(l->mutex);
}

Lishi* state_init() {
    Lishi* l = malloc(sizeof(Lishi));
    l->input_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    l->view_port = view_port_alloc();
    l->gui = furi_record_open(RECORD_GUI);
    l->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    l->notifications = furi_record_open(RECORD_NOTIFICATION);
    for(int i = 0; i < MAX_KEYS; i++) {
        for(int j = 0; j < OTHER_COLUMNS; j++) {
            l->counts[i][j] = 0;
        }
    }
    l->active_column = 0;
    l->menu_state = MenuMain;
    l->selected_menu = 0;
    l->selected_key = 0;
    memset(l->current_key, 0, sizeof(l->current_key));
    view_port_input_callback_set(l->view_port, input_callback, l);
    view_port_draw_callback_set(l->view_port, render_callback, l);
    gui_add_view_port(l->gui, l->view_port, GuiLayerFullscreen);
    return l;
}

void save_lishi_values(Lishi* l) {
    Storage* storage = (Storage*)furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    if(storage_file_open(file, SAVE_FILE_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        storage_file_write(file, l->counts, sizeof(l->counts));
        storage_file_close(file);
    }
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}

void load_lishi_values(Lishi* l) {
    Storage* storage = (Storage*)furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    if(storage_file_open(file, SAVE_FILE_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_read(file, l->counts, sizeof(l->counts));
        storage_file_close(file);
    }
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}

int32_t lishiapp(void) {
    Lishi* l = state_init();
    load_lishi_values(l);

    InputEvent input;
    while(true) {
        while(furi_message_queue_get(l->input_queue, &input, FuriWaitForever) == FuriStatusOk) {
            furi_check(furi_mutex_acquire(l->mutex, FuriWaitForever) == FuriStatusOk);

            if(input.type == InputTypeShort) {
                switch(input.key) {
                case InputKeyBack:
                    if(l->menu_state == MenuMain) {
                        save_lishi_values(l);
                        furi_mutex_release(l->mutex);
                        state_free(l);
                        return 0;
                    } else {
                        l->menu_state = MenuMain;
                    }
                    break;
                case InputKeyDown:
                    if(l->menu_state == MenuMain) {
                        l->selected_menu = (l->selected_menu + 1) % 3;
                    } else if(l->menu_state == MenuConfig) {
                        l->selected_key = (l->selected_key + 1) % MAX_KEYS;
                    } else if(l->menu_state == MenuShow) {
                        if((l->selected_key == 3 || l->selected_key == 5 ||
                            l->selected_key == 4) &&
                           l->counts[l->selected_key][l->active_column] < 5) {
                            l->counts[l->selected_key][l->active_column]++;
                        } else if(
                            l->selected_key != 3 && l->selected_key != 5 && l->selected_key != 4 &&
                            l->counts[l->selected_key][l->active_column] < MAX_COUNT) {
                            l->counts[l->selected_key][l->active_column]++;
                        }
                    }
                    break;
                case InputKeyUp:
                    if(l->menu_state == MenuMain) {
                        l->selected_menu = (l->selected_menu - 1 + 3) % 3;
                    } else if(l->menu_state == MenuConfig) {
                        l->selected_key = (l->selected_key - 1 + MAX_KEYS) % MAX_KEYS;
                    } else if(l->menu_state == MenuShow) {
                        if((l->selected_key == 3 || l->selected_key == 5 ||
                            l->selected_key == 4) &&
                           l->counts[l->selected_key][l->active_column] > 0) {
                            l->counts[l->selected_key][l->active_column]--;
                        } else if(
                            l->selected_key != 3 && l->selected_key != 5 && l->selected_key != 4 &&
                            l->counts[l->selected_key][l->active_column] > 0) {
                            l->counts[l->selected_key][l->active_column]--;
                        }
                    }
                    break;
                case InputKeyLeft:
                    if(l->menu_state == MenuShow) {
                        l->active_column =
                            (l->active_column - 1 +
                             (l->selected_key == 4 ? TOY48_COLUMNS : OTHER_COLUMNS)) %
                            (l->selected_key == 4 ? TOY48_COLUMNS : OTHER_COLUMNS);
                    }
                    break;
                case InputKeyRight:
                    if(l->menu_state == MenuShow) {
                        l->active_column = (l->active_column + 1) %
                                           (l->selected_key == 4 ? TOY48_COLUMNS : OTHER_COLUMNS);
                    }
                    break;
                case InputKeyOk:
                    if(l->menu_state == MenuMain) {
                        if(l->selected_menu == 0)
                            l->menu_state = MenuConfig;
                        else if(l->selected_menu == 1)
                            l->menu_state = MenuShow;
                        else if(l->selected_menu == 2)
                            l->menu_state = MenuAbout;
                    } else if(l->menu_state == MenuConfig) {
                        const char* keys[MAX_KEYS] = {
                            "HU66", "HU92", "HU83", "K5", "TOY48", "FO38"};
                        strncpy(l->current_key, keys[l->selected_key], sizeof(l->current_key) - 1);
                        l->menu_state = MenuMain;
                    }
                    break;
                default:
                    break;
                }
            }

            furi_mutex_release(l->mutex);
            view_port_update(l->view_port);
        }
    }
}
