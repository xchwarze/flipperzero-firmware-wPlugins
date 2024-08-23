#include "insults_scene_saved.h"

void insults_scene_saved_on_enter(void* context) {
    InsultsApp* insults_app = (InsultsApp*)context;

    insults_app->saved_lines = file_lines_alloc();

    insults_files_get_lines(
        insults_app->insults_files, APP_DATA_PATH("saved_insults.txt"), insults_app->saved_lines);

    for(unsigned int i = 0; i < insults_app->saved_lines->num_lines; i++) {
        submenu_add_item(
            insults_app->saved_menu,
            strdup(insults_app->saved_lines->lines[i]),
            i,
            insults_scene_saved_submenu_callback,
            insults_app);
    }

    if(insults_app->saved_lines->num_lines == 0) {
        view_dispatcher_switch_to_view(insults_app->view_dispatcher, InsultsViewSavedEmpty);
    }

    else {
        unsigned int index =
            scene_manager_get_scene_state(insults_app->scene_manager, InsultsSceneSaved);

        if(index >= insults_app->saved_lines->num_lines) {
            submenu_set_selected_item(
                insults_app->saved_menu, insults_app->saved_lines->num_lines - 1);
        }

        else {
            submenu_set_selected_item(insults_app->saved_menu, index);
        }

        view_dispatcher_switch_to_view(insults_app->view_dispatcher, InsultsViewSaved);
    }
}

bool insults_scene_saved_on_event(void* context, SceneManagerEvent event) {
    InsultsApp* insults_app = (InsultsApp*)context;

    if(event.type == SceneManagerEventTypeCustom) {
        insults_app->insult_string = strdup(insults_app->saved_lines->lines[event.event]);

        scene_manager_set_scene_state(insults_app->scene_manager, InsultsSceneSaved, event.event);

        scene_manager_next_scene(insults_app->scene_manager, InsultsSceneSavedDetail);

        return true;
    }

    return false;
}

void insults_scene_saved_on_exit(void* context) {
    InsultsApp* insults_app = (InsultsApp*)context;

    submenu_reset(insults_app->saved_menu);

    file_lines_free(insults_app->saved_lines);
}

void insults_scene_saved_submenu_callback(void* context, uint32_t index) {
    InsultsApp* insults_app = (InsultsApp*)context;

    view_dispatcher_send_custom_event(insults_app->view_dispatcher, index);
}
