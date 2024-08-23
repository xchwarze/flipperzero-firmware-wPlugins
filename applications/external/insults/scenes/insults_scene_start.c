#include "insults_scene_start.h"

void insults_scene_start_on_enter(void* context) {
    InsultsApp* insults_app = (InsultsApp*)context;

    view_dispatcher_switch_to_view(insults_app->view_dispatcher, InsultsViewStart);
}

bool insults_scene_start_on_event(void* context, SceneManagerEvent event) {
    InsultsApp* insults_app = (InsultsApp*)context;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == InsultsSceneStartIndexNew) {
            scene_manager_set_scene_state(
                insults_app->scene_manager, InsultsSceneStart, event.event);

            scene_manager_next_scene(insults_app->scene_manager, InsultsSceneInsult);

            return true;
        }

        if(event.event == InsultsSceneStartIndexSaved) {
            scene_manager_set_scene_state(
                insults_app->scene_manager, InsultsSceneStart, event.event);
            scene_manager_set_scene_state(insults_app->scene_manager, InsultsSceneSaved, 0);

            scene_manager_next_scene(insults_app->scene_manager, InsultsSceneSaved);

            return true;
        }

        if(event.event == InsultsSceneStartIndexClear) {
            insults_files_clear_insults(insults_app->insults_files);

            return true;
        }
    }

    return false;
}

void insults_scene_start_on_exit(void* context) {
    UNUSED(context);
}

void insults_scene_start_submenu_callback(void* context, uint32_t index) {
    InsultsApp* insults_app = (InsultsApp*)context;

    view_dispatcher_send_custom_event(insults_app->view_dispatcher, index);
}
