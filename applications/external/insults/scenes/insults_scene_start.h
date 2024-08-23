#pragma once

#include <gui/scene_manager.h>
#include <gui/view_dispatcher.h>

#include "../insults_app.h"
#include "../insults_files.h"

enum InsultsSceneStartIndex {
    InsultsSceneStartIndexNew,
    InsultsSceneStartIndexSaved,
    InsultsSceneStartIndexClear
};

void insults_scene_start_on_enter(void* context);
bool insults_scene_start_on_event(void* context, SceneManagerEvent event);
void insults_scene_start_on_exit(void* context);

void insults_scene_start_submenu_callback(void* context, uint32_t index);
