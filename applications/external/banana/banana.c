#include <furi.h>
#include <furi_hal.h>

#include <gui/gui.h>
#include <input/input.h>
#include <storage/storage.h>

#include "banana.h"
#include "banana_icons.h"

static void save_state(AppState* state) {
    File* file = storage_file_alloc(furi_record_open(RECORD_STORAGE));

    if(storage_file_open(file, APP_DATA_PATH("save.txt"), FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        storage_file_write(file, state, sizeof(AppState));
    }

    storage_file_close(file);
    storage_file_free(file);

    furi_record_close(RECORD_STORAGE);
}

static void load_state(AppState* state) {
    File* file = storage_file_alloc(furi_record_open(RECORD_STORAGE));

    if(storage_file_open(file, APP_DATA_PATH("save.txt"), FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_read(file, state, sizeof(AppState));
    }

    storage_file_close(file);
    storage_file_free(file);

    furi_record_close(RECORD_STORAGE);
}

static void draw_callback(Canvas* canvas, void* ctx) {
    furi_assert(ctx);
    AppState* state = ctx;

    canvas_clear(canvas);

    // Draw banana image
    if(state->inverted) {
        canvas_draw_icon(canvas, 32, 0, &I_banana_black_64_64);
    } else {
        canvas_draw_icon(canvas, 32, 0, &I_banana_64_64);
    }

    // Draw counter
    char counter_str[16];
    snprintf(counter_str, sizeof(counter_str), "%lu", state->counter);
    canvas_draw_str_aligned(canvas, 64, 64, AlignCenter, AlignBottom, counter_str);
}

static void input_callback(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);

    FuriMessageQueue* event_queue = ctx;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

int32_t banana_main(void* p) {
    UNUSED(p);

    AppState state = {
        .counter = 0,
        .inverted = false,
    };

    load_state(&state);

    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, draw_callback, &state);
    view_port_input_callback_set(view_port, input_callback, event_queue);

    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    InputEvent event;

    bool running = true;
    while(running) {
        if(furi_message_queue_get(event_queue, &event, 100) == FuriStatusOk) {
            if(event.type == InputTypePress) {
                switch(event.key) {
                case InputKeyOk:
                    state.counter++;
                    state.inverted = true;
                    view_port_update(view_port);
                    furi_hal_vibro_on(true);
                    furi_delay_ms(100);
                    furi_hal_vibro_on(false);
                    state.inverted = false;
                    break;
                case InputKeyBack:
                    running = false;
                    break;
                default:
                    break;
                }
            }
        }
        view_port_update(view_port);
    }

    save_state(&state);

    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);

    furi_record_close(RECORD_GUI);

    return 0;
}
