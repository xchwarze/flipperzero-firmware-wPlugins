#include "marmalade_app.h"
#include <furi_hal_region.h>

static FuriHalRegion unlockedRegion = {
    .country_code = "FTW",
    .bands_count = 3,
    .bands =
        {
            {.start = 299999755, .end = 348000000, .power_limit = 12, .duty_cycle = 50},
            {.start = 386999938, .end = 464000000, .power_limit = 12, .duty_cycle = 50},
            {.start = 778999847, .end = 928000000, .power_limit = 12, .duty_cycle = 50},
        },
};

static const char* marmalade_modes[] = {
    "OOK 650kHz",
    "2FSK 2.38kHz",
    "2FSK 47.6kHz",
    "MSK 99.97Kb/s",
    "GFSK 9.99Kb/s",
    "Bruteforce 0xFF"};

typedef struct {
    uint32_t min;
    uint32_t max;
} FrequencyBand;

static const FrequencyBand valid_frequency_bands[] = {
    {300000000, 348000000}, // Band 1
    {387000000, 464000000}, // Band 2
    {779000000, 928000000} // Band 3
};

#define NUM_FREQUENCY_BANDS (sizeof(valid_frequency_bands) / sizeof(valid_frequency_bands[0]))

static bool is_frequency_valid(uint32_t frequency) {
    for(size_t i = 0; i < NUM_FREQUENCY_BANDS; i++) {
        if(frequency >= valid_frequency_bands[i].min &&
           frequency <= valid_frequency_bands[i].max) {
            return true;
        }
    }
    return false;
}

static uint32_t adjust_frequency_to_valid(uint32_t frequency, bool up) {
    if(is_frequency_valid(frequency)) {
        return frequency;
    } else {
        if(up) {
            for(size_t i = 0; i < NUM_FREQUENCY_BANDS; i++) {
                if(frequency < valid_frequency_bands[i].min) {
                    return valid_frequency_bands[i].min;
                }
            }
            return valid_frequency_bands[0].min;
        } else {
            for(int i = NUM_FREQUENCY_BANDS - 1; i >= 0; i--) {
                if(frequency > valid_frequency_bands[i].max) {
                    return valid_frequency_bands[i].max;
                }
            }
            return valid_frequency_bands[NUM_FREQUENCY_BANDS - 1].max;
        }
    }
}

static void marmalade_draw_callback(Canvas* canvas, void* context) {
    MarmaladeApp* app = (MarmaladeApp*)context;
    canvas_clear(canvas);

    char freq_str[20];
    snprintf(
        freq_str,
        sizeof(freq_str),
        "%3lu.%02lu",
        app->frequency / 1000000,
        (app->frequency % 1000000) / 10000);

    int total_width = strlen(freq_str) * 12;
    int start_x = (128 - total_width) / 2;
    int digit_position = 0;

    for(size_t i = 0; i < strlen(freq_str); i++) {
        bool highlight = (digit_position == app->cursor_position);

        if(freq_str[i] != '.') {
            canvas_set_font(canvas, highlight ? FontBigNumbers : FontPrimary);
            char temp[2] = {freq_str[i], '\0'};
            canvas_draw_str_aligned(canvas, start_x + (i * 12), 10, AlignCenter, AlignTop, temp);
            digit_position++;
        } else {
            canvas_set_font(canvas, FontPrimary);
            char temp[2] = {freq_str[i], '\0'};
            canvas_draw_str_aligned(canvas, start_x + (i * 12), 10, AlignCenter, AlignTop, temp);
        }
    }

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(
        canvas, 64, 55, AlignCenter, AlignTop, marmalade_modes[app->marmalade_mode]);
}

static void marmalade_input_callback(InputEvent* input_event, void* context) {
    MarmaladeApp* app = (MarmaladeApp*)context;
    furi_message_queue_put(app->event_queue, input_event, FuriWaitForever);
}

static void marmalade_adjust_frequency(MarmaladeApp* app, bool up) {
    uint32_t frequency = app->frequency;
    uint32_t step;

    switch(app->cursor_position) {
    case 0:
        step = 100000000;
        break;
    case 1:
        step = 10000000;
        break;
    case 2:
        step = 1000000;
        break;
    case 3:
        step = 100000;
        break;
    case 4:
        step = 10000;
        break;
    default:
        return;
    }

    frequency = up ? frequency + step : frequency - step;

    if(frequency > SUBGHZ_FREQUENCY_MAX) {
        frequency = SUBGHZ_FREQUENCY_MIN;
    } else if(frequency < SUBGHZ_FREQUENCY_MIN) {
        frequency = SUBGHZ_FREQUENCY_MAX;
    }

    frequency = adjust_frequency_to_valid(frequency, up);

    app->frequency = frequency;

    if(app->tx_running) {
        subghz_tx_rx_worker_stop(app->subghz_txrx);
        subghz_tx_rx_worker_start(app->subghz_txrx, app->device, app->frequency);
    }
}

static int32_t marmalade_tx_thread(void* context) {
    MarmaladeApp* app = context;
    uint8_t jam_data[MESSAGE_MAX_LEN];

    switch(app->marmalade_mode) {
    case MarmaladeModeOok650Async:
        memset(jam_data, 0xFF, sizeof(jam_data));
        break;
    case MarmaladeMode2FSKDev238Async:
    case MarmaladeMode2FSKDev476Async:
        for(size_t i = 0; i < sizeof(jam_data); i++) {
            jam_data[i] = (i % 2 == 0) ? 0xAA : 0x55;
        }
        break;
    case MarmaladeModeMSK99_97KbAsync:
    case MarmaladeModeGFSK9_99KbAsync:
        for(size_t i = 0; i < sizeof(jam_data); i++) {
            jam_data[i] = rand() % 256;
        }
        break;
    case MarmaladeModeBruteforce:
        memset(jam_data, 0xFF, sizeof(jam_data));
        break;
    }

    while(app->tx_running) {
        while(!subghz_tx_rx_worker_write(app->subghz_txrx, jam_data, sizeof(jam_data))) {
            furi_delay_ms(20);
        }
        furi_delay_ms(10);
    }

    return 0;
}

static void marmalade_switch_mode(MarmaladeApp* app) {
    app->tx_running = false;

    if(app->tx_thread) {
        furi_thread_join(app->tx_thread);
        furi_thread_free(app->tx_thread);
        app->tx_thread = NULL;
    }

    if(subghz_tx_rx_worker_is_running(app->subghz_txrx)) {
        subghz_tx_rx_worker_stop(app->subghz_txrx);
    }

    app->marmalade_mode = (app->marmalade_mode + 1) % 6;

    switch(app->marmalade_mode) {
    case MarmaladeModeOok650Async:
        subghz_devices_load_preset(app->device, FuriHalSubGhzPresetOok650Async, NULL);
        break;
    case MarmaladeMode2FSKDev238Async:
        subghz_devices_load_preset(app->device, FuriHalSubGhzPreset2FSKDev238Async, NULL);
        break;
    case MarmaladeMode2FSKDev476Async:
        subghz_devices_load_preset(app->device, FuriHalSubGhzPreset2FSKDev476Async, NULL);
        break;
    case MarmaladeModeMSK99_97KbAsync:
        subghz_devices_load_preset(app->device, FuriHalSubGhzPresetMSK99_97KbAsync, NULL);
        break;
    case MarmaladeModeGFSK9_99KbAsync:
        subghz_devices_load_preset(app->device, FuriHalSubGhzPresetGFSK9_99KbAsync, NULL);
        break;
    case MarmaladeModeBruteforce:
        subghz_devices_load_preset(app->device, FuriHalSubGhzPresetOok650Async, NULL);
        break;
    default:
        return;
    }

    subghz_tx_rx_worker_start(app->subghz_txrx, app->device, app->frequency);

    app->tx_running = true;
    app->tx_thread = furi_thread_alloc();
    furi_thread_set_name(app->tx_thread, "Marmalade TX");
    furi_thread_set_stack_size(app->tx_thread, 2048);
    furi_thread_set_context(app->tx_thread, app);
    furi_thread_set_callback(app->tx_thread, marmalade_tx_thread);
    furi_thread_start(app->tx_thread);
}

static void marmalade_update_view(MarmaladeApp* app) {
    view_port_update(app->view_port);
}

static bool marmalade_init_subghz(MarmaladeApp* app) {
    app->device = subghz_devices_get_by_name(SUBGHZ_DEVICE_NAME);
    if(!app->device) {
        return false;
    }

    subghz_devices_load_preset(app->device, FuriHalSubGhzPresetOok650Async, NULL);

    if(!subghz_tx_rx_worker_start(app->subghz_txrx, app->device, app->frequency)) {
        return false;
    }

    return true;
}

static void marmalade_splash_screen_draw_callback(Canvas* canvas, void* context) {
    UNUSED(context);

    canvas_clear(canvas);

    for(int x = 0; x < 128; x += 8) {
        for(int y = 0; y < 64; y += 8) {
            canvas_draw_dot(canvas, x, y);
        }
    }

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 5, AlignCenter, AlignTop, "RF Marmalade");
    canvas_draw_str_aligned(canvas, 64, 15, AlignCenter, AlignTop, "by RocketGod");
    canvas_draw_str_aligned(canvas, 64, 30, AlignCenter, AlignTop, "For Educational Testing");
    canvas_draw_str_aligned(canvas, 64, 45, AlignCenter, AlignTop, "in a Dedicated Lab Only");
    canvas_draw_frame(canvas, 0, 0, 128, 64);
}

static void marmalade_show_splash_screen(MarmaladeApp* app) {
    view_port_draw_callback_set(app->view_port, marmalade_splash_screen_draw_callback, app);
    view_port_update(app->view_port);
    furi_delay_ms(2000);
    view_port_draw_callback_set(app->view_port, marmalade_draw_callback, app);
}

MarmaladeApp* marmalade_app_alloc(void) {
    MarmaladeApp* app = malloc(sizeof(MarmaladeApp));
    if(!app) {
        return NULL;
    }

    app->view_port = view_port_alloc();
    app->event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    app->frequency = 315000000;
    app->cursor_position = 0;
    app->running = true;
    app->tx_running = false;
    app->marmalade_mode = MarmaladeModeOok650Async;
    app->gui = furi_record_open(RECORD_GUI);

    furi_hal_region_set(&unlockedRegion);

    view_port_draw_callback_set(app->view_port, marmalade_draw_callback, app);
    view_port_input_callback_set(app->view_port, marmalade_input_callback, app);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    app->tx_thread = NULL;
    app->device = NULL;

    subghz_devices_init();
    app->subghz_txrx = subghz_tx_rx_worker_alloc();

    furi_hal_power_suppress_charge_enter();

    return app;
}

void marmalade_app_free(MarmaladeApp* app) {
    app->tx_running = false;
    if(app->tx_thread) {
        furi_thread_join(app->tx_thread);
        furi_thread_free(app->tx_thread);
        app->tx_thread = NULL;
    }

    if(subghz_tx_rx_worker_is_running(app->subghz_txrx)) {
        subghz_tx_rx_worker_stop(app->subghz_txrx);
    }
    subghz_tx_rx_worker_free(app->subghz_txrx);
    subghz_devices_deinit();

    furi_hal_power_suppress_charge_exit();

    gui_remove_view_port(app->gui, app->view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(app->view_port);
    furi_message_queue_free(app->event_queue);

    free(app);
}

int32_t marmalade_app(void* p) {
    UNUSED(p);

    MarmaladeApp* app = marmalade_app_alloc();
    if(!app) {
        return -1;
    }

    marmalade_show_splash_screen(app);

    if(!marmalade_init_subghz(app)) {
        marmalade_app_free(app);
        return -1;
    }

    InputEvent event;
    while(app->running) {
        if(furi_message_queue_get(app->event_queue, &event, 10) == FuriStatusOk) {
            if(event.type == InputTypeShort) {
                switch(event.key) {
                case InputKeyOk:
                    marmalade_switch_mode(app);
                    marmalade_update_view(app);
                    break;
                case InputKeyBack:
                    app->running = false;
                    break;
                case InputKeyRight:
                    if(app->cursor_position < 4) {
                        app->cursor_position++;
                        marmalade_update_view(app);
                    }
                    break;
                case InputKeyLeft:
                    if(app->cursor_position > 0) {
                        app->cursor_position--;
                        marmalade_update_view(app);
                    }
                    break;
                case InputKeyUp:
                    marmalade_adjust_frequency(app, true);
                    marmalade_update_view(app);
                    break;
                case InputKeyDown:
                    marmalade_adjust_frequency(app, false);
                    marmalade_update_view(app);
                    break;
                default:
                    break;
                }
            }
        }
    }

    marmalade_app_free(app);

    return 0;
}
