#include <storage/storage.h>

#include "upython.h"

void upython_cli(Cli* cli, FuriString* args, void* ctx) {
    UNUSED(ctx);

    if(action != ActionNone) {
        printf("%s is busy!\n", TAG);

        return;
    }

    if(furi_string_empty(args)) {
        action = ActionRepl;

        upython_repl_execute(cli);

        action = ActionNone;
    } else {
        furi_string_set(file_path, args);

        action = ActionExec;
    }
}

void upython_cli_register(void* args) {
    if(args != NULL) {
        file_path = furi_string_alloc_set_str(args);

        action = ActionExec;

        return;
    } else {
        file_path = furi_string_alloc();

        upython_reset_file_path();

        action = ActionNone;
    }

    Cli* cli = furi_record_open(RECORD_CLI);

    cli_add_command(cli, CLI, CliCommandFlagParallelSafe, upython_cli, NULL);

    furi_record_close(RECORD_CLI);
}

void upython_cli_unregister(void* args) {
    furi_string_free(file_path);

    if(args != NULL) {
        return;
    }

    Cli* cli = furi_record_open(RECORD_CLI);

    cli_delete_command(cli, CLI);

    furi_record_close(RECORD_CLI);
}
