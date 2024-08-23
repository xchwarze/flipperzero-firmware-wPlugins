#include <gui/scene_manager.h>
#include <gui/view_dispatcher.h>

#include "insults_app.h"
#include "scenes/insults_scene.h"

int32_t insults_app(void* p) {
    UNUSED(p);

    InsultsApp* app = insults_app_alloc();

    scene_manager_next_scene(app->scene_manager, InsultsSceneStart);

    view_dispatcher_run(app->view_dispatcher);

    insults_app_free(app);

    return 0;
}
