#pragma once

#include <furi/core/record.h>
#include <gui/canvas.h>
#include <gui/gui.h>
#include <gui/scene_manager.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/dialog_ex.h>

#include "insults_files.h"
#include "scenes/insults_scene.h"
#include "scenes/insults_scene_start.h"

enum InsultsView {
    InsultsViewStart,
    InsultsViewInsult,
    InsultsViewSaved,
    InsultsViewSavedDetail,
    InsultsViewSavedEmpty
};

typedef struct InsultsApp {
    Gui* gui;
    SceneManager* scene_manager;
    ViewDispatcher* view_dispatcher;
    InsultsFiles* insults_files;
    Submenu* start_menu;
    DialogEx* insult_display;
    Submenu* saved_menu;
    DialogEx* saved_detail;
    FileLines* saved_lines;
    DialogEx* saved_empty;
    char* insult_string;
} InsultsApp;

InsultsApp* insults_app_alloc(void);

void insults_app_free(InsultsApp* insults_app);

bool insults_navigation_event_callback(void* context);

bool insults_custom_event_callback(void* context, uint32_t event);
