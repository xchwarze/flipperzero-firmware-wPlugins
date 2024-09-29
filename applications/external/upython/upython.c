#include <furi.h>
#include <storage/storage.h>

#include <mp_flipper_runtime.h>
#include <mp_flipper_compiler.h>

#include "upython.h"

Action action = ActionNone;
FuriString* file_path = NULL;

void upython_reset_file_path() {
    furi_string_set(file_path, APP_ASSETS_PATH("upython"));
}

int32_t upython(void* args) {
    upython_cli_register(args);

    do {
        switch(action) {
        case ActionNone:
            action = upython_splash_screen();

            break;
        case ActionOpen:
            if(upython_select_python_file(file_path)) {
                action = ActionExec;
            } else {
                upython_reset_file_path();

                action = ActionNone;
            }

            break;
        case ActionRepl:
            break;
        case ActionExec:
            upython_file_execute(file_path);

            upython_reset_file_path();

            action = ActionNone;

            break;
        case ActionExit:
            action = upython_confirm_exit_action() ? ActionTerm : ActionNone;
            break;
        case ActionTerm:
            break;
        }

        furi_delay_ms(1);
    } while(action != ActionTerm);

    upython_cli_unregister(args);

    return 0;
}
