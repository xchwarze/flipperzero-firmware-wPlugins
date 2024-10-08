#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <subghz/devices/devices.h>
#include <subghz/devices/preset.h>
#include <furi/core/log.h>
#include <furi_hal.h>
#include <lib/subghz/subghz_tx_rx_worker.h>

#define TAG                  "MarmaladeApp"
#define SUBGHZ_DEVICE_NAME   "cc1101_int"
#define SUBGHZ_FREQUENCY_MIN 300000000
#define SUBGHZ_FREQUENCY_MAX 928000000
#define MESSAGE_MAX_LEN      256

typedef enum {
    MarmaladeModeOok650Async,
    MarmaladeMode2FSKDev238Async,
    MarmaladeMode2FSKDev476Async,
    MarmaladeModeMSK99_97KbAsync,
    MarmaladeModeGFSK9_99KbAsync,
    MarmaladeModeBruteforce,
} MarmaladeMode;

typedef enum {
    MarmaladeStateSplash,
    MarmaladeStateMain
} MarmaladeState;

typedef struct {
    Gui* gui;
    ViewPort* view_port;
    FuriMessageQueue* event_queue;
    uint32_t frequency;
    uint8_t cursor_position;
    bool running;
    MarmaladeMode marmalade_mode;
    const SubGhzDevice* device;
    SubGhzTxRxWorker* subghz_txrx;
    FuriThread* tx_thread;
    bool tx_running;
    MarmaladeState state;
} MarmaladeApp;

MarmaladeApp* marmalade_app_alloc(void);
void marmalade_app_free(MarmaladeApp* app);
int32_t marmalade_app(void* p);
