#include "insults_scene.h"

void (*const on_enter_handlers[])(void*) = {
    insults_scene_start_on_enter,
    insults_scene_insult_on_enter,
    insults_scene_saved_on_enter,
    insults_scene_saved_detail_on_enter};
bool (*const on_event_handlers[])(void*, SceneManagerEvent) = {
    insults_scene_start_on_event,
    insults_scene_insult_on_event,
    insults_scene_saved_on_event,
    insults_scene_saved_detail_on_event};
void (*const on_exit_handlers[])(void*) = {
    insults_scene_start_on_exit,
    insults_scene_insult_on_exit,
    insults_scene_saved_on_exit,
    insults_scene_saved_detail_on_exit};

const SceneManagerHandlers scene_handlers =
    {on_enter_handlers, on_event_handlers, on_exit_handlers, 4};
