#include "insults_scene_saved_detail.h"

void insults_scene_saved_detail_on_enter(void* context) {
    InsultsApp* insults_app = (InsultsApp*)context;

    dialog_ex_set_text(
        insults_app->saved_detail, insults_app->insult_string, 64, 32, AlignCenter, AlignCenter);

    view_dispatcher_switch_to_view(insults_app->view_dispatcher, InsultsViewSavedDetail);
}

bool insults_scene_saved_detail_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);

    return false;
}

void insults_scene_saved_detail_on_exit(void* context) {
    InsultsApp* insults_app = (InsultsApp*)context;

    free(insults_app->insult_string);
}

void insults_scene_saved_detail_dialog_callback(DialogExResult result, void* context) {
    InsultsApp* insults_app = (InsultsApp*)context;

    if(result == DialogExResultLeft) {
        insults_files_delete_insult(
            insults_app->insults_files,
            scene_manager_get_scene_state(insults_app->scene_manager, InsultsSceneSaved));

        scene_manager_handle_back_event(insults_app->scene_manager);
    }
}
