#include "insults_scene_insult.h"

void insults_scene_insult_on_enter(void* context) {
    InsultsApp* insults_app = (InsultsApp*)context;

    insults_scene_insult_set_text(insults_app);

    view_dispatcher_switch_to_view(insults_app->view_dispatcher, InsultsViewInsult);
}

bool insults_scene_insult_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);

    return false;
}

void insults_scene_insult_on_exit(void* context) {
    InsultsApp* insults_app = (InsultsApp*)context;

    free(insults_app->insult_string);
}

void insults_scene_insult_dialog_callback(DialogExResult result, void* context) {
    InsultsApp* insults_app = (InsultsApp*)context;

    if(result == DialogExResultLeft) {
        insults_scene_insult_set_text(insults_app);
    }

    else if(result == DialogExResultRight) {
        insults_files_save_insult(insults_app->insults_files, insults_app->insult_string);
        insults_scene_insult_set_text(insults_app);
    }
}

void insults_scene_insult_set_text(void* context) {
    InsultsApp* insults_app = (InsultsApp*)context;

    if(insults_app->insult_string) {
        free(insults_app->insult_string);
    }

    insults_app->insult_string = insults_files_get_insult(insults_app->insults_files);

    dialog_ex_set_text(
        insults_app->insult_display, insults_app->insult_string, 64, 32, AlignCenter, AlignCenter);
}
