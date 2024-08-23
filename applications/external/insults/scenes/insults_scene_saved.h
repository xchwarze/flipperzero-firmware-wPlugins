#pragma once

#include <string.h>

#include <gui/scene_manager.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <storage/storage.h>

#include "../insults_app.h"
#include "../insults_files.h"

void insults_scene_saved_on_enter(void* context);
bool insults_scene_saved_on_event(void* context, SceneManagerEvent event);
void insults_scene_saved_on_exit(void* context);

void insults_scene_saved_submenu_callback(void* context, uint32_t index);
