#include "../../app.h"
#include "../../structs.h"
#include "./build_http_call.h"
#include <furi.h>
#include <furi_hal.h>
#include <gui/modules/variable_item_list.h>
#include "../../csv/csv_utils/csv_utils.h"
#include "../../csv/csv_build_url/csv_build_url.h"

static void build_http_call_select_callback(void* context, uint32_t index) {
    App* app = context;
    furi_assert(app);
    furi_assert(app->view_dispatcher);
    bool url_found = url_in_csv(app, app->build_http_state->url, StateTypeBuildHttp);

    switch(index) {
    case 0: // Mode: Display / Save to file
        view_dispatcher_send_custom_event(app->view_dispatcher, BuildHttpItemMode);
        break;
    case 1: // Method: Get / Head / etc ...
        view_dispatcher_send_custom_event(app->view_dispatcher, BuildHttpItemMethod);
        break;
    case 2: // Show response headers
        view_dispatcher_send_custom_event(app->view_dispatcher, BuildHttpItemShowHeaders);
        break;
    case 3: // Set Url
        view_dispatcher_send_custom_event(app->view_dispatcher, BuildHttpItemSetUrl);
        break;
    case 4: // Set headers
        view_dispatcher_send_custom_event(app->view_dispatcher, BuildHttpItemSetHeaders);
        break;
    case 5: // Set Payload
        view_dispatcher_send_custom_event(app->view_dispatcher, BuildHttpItemSetPayload);
        break;
    case 6: // This will be either action or load from csv
        if(strlen(app->build_http_state->url) > 0) {
            view_dispatcher_send_custom_event(app->view_dispatcher, BuildHttpItemAction);
        } else {
            view_dispatcher_send_custom_event(app->view_dispatcher, BuildHttpItemLoadFromCsv);
        }
        break;
    case 7:
        if(url_found) {
            view_dispatcher_send_custom_event(app->view_dispatcher, BuildHttpItemDeleteFromCsv);
        } else {
            view_dispatcher_send_custom_event(app->view_dispatcher, BuildHttpItemSaveToCsv);
        }
        break;
    case 8:
        view_dispatcher_send_custom_event(app->view_dispatcher, BuildHttpItemLoadFromCsv);
        break;
    default:
        break;
    }
}

static void build_http_call_mode_callback(VariableItem* item) {
    App* app = variable_item_get_context(item);
    furi_assert(app);
    app->build_http_state->mode = !app->build_http_state->mode;
    draw_build_http_call_menu(app);
}

static void build_http_call_method_callback(VariableItem* item) {
    App* app = variable_item_get_context(item);
    furi_assert(app);
    app->build_http_state->http_method = (app->build_http_state->http_method + 1) % 6;
    draw_build_http_call_menu(app);
}

static void build_http_call_show_headers_callback(VariableItem* item) {
    App* app = variable_item_get_context(item);
    furi_assert(app);
    app->build_http_state->show_response_headers = !app->build_http_state->show_response_headers;
    draw_build_http_call_menu(app);
}

void draw_build_http_call_menu(App* app) {
    furi_assert(app);
    furi_assert(app->variable_item_list);
    VariableItemList* variable_item_list = app->variable_item_list;

    variable_item_list_reset(variable_item_list);

    VariableItem* item;

    bool url_set = strlen(app->build_http_state->url) > 0;
    bool payload_set = !furi_string_empty(app->build_http_state->payload);
    bool url_found = url_in_csv(app, app->build_http_state->url, StateTypeBuildHttp);

    FURI_LOG_T(TAG, "URL found %d", url_found);
    // Mode selection: Display / Save to file
    const char* mode_names[] = {"Display", "Save"};
    item =
        variable_item_list_add(variable_item_list, "Mode", 2, build_http_call_mode_callback, app);
    variable_item_set_current_value_text(item, mode_names[app->build_http_state->mode]);

    // Http Method Selection
    const char* method_names[] = {"HEAD", "GET", "POST", "PATCH", "PUT", "DELETE"};
    item = variable_item_list_add(
        variable_item_list, "HTTP Method", 6, build_http_call_method_callback, app);
    variable_item_set_current_value_text(item, method_names[app->build_http_state->http_method]);

    // Show response headers switch
    item = variable_item_list_add(
        variable_item_list, "Show Response Headers", 2, build_http_call_show_headers_callback, app);
    variable_item_set_current_value_text(
        item, app->build_http_state->show_response_headers ? "On" : "Off");

    // Set URL button
    item =
        variable_item_list_add(variable_item_list, url_set ? "Edit Url" : "Set URL", 0, NULL, app);

    // Set Headers button
    item = variable_item_list_add(variable_item_list, "Set Headers", 0, NULL, app);

    // Set Payload button
    item = variable_item_list_add(
        variable_item_list, payload_set ? "Edit Payload" : "Set Payload", 0, NULL, app);

    // Send request button (show if url is not empty)
    if(url_set) {
        item = variable_item_list_add(
            variable_item_list,
            app->build_http_state->mode ? "Save to File" : "Send Request",
            0,
            NULL,
            app);
    }

    if(url_set) {
        item = variable_item_list_add(
            variable_item_list, url_found ? "Delete from CSV" : "Save to CSV", 0, NULL, app);
    }
    if(strlen(app->build_http_list[0].url) > 0) {
        item = variable_item_list_add(variable_item_list, "Load from CSV", 0, NULL, app);
    }
}

void scene_on_enter_build_http_call(void* context) {
    App* app = context;
    furi_assert(app);
    furi_assert(app->variable_item_list);

    if(!app->build_http_state) {
        app->build_http_state = malloc(sizeof(BuildHttpState));
        app->build_http_state->mode = false;
        app->build_http_state->http_method = GET;
        app->build_http_state->show_response_headers = false;
        strcpy(app->build_http_state->url, "");
        app->build_http_state->payload = furi_string_alloc();
    }

    variable_item_list_set_enter_callback(
        app->variable_item_list, build_http_call_select_callback, app);
    draw_build_http_call_menu(app);
    variable_item_list_set_selected_item(app->variable_item_list, 0);
    view_dispatcher_switch_to_view(app->view_dispatcher, AppView_BuildHttpCall);
}

bool scene_on_event_build_http_call(void* context, SceneManagerEvent event) {
    App* app = context;
    furi_assert(app);
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case BuildHttpItemSetUrl: // Set URL
            app->selected_tx_string = app->build_http_state->url;
            app->text_input_state = TextInputState_BuildHttpUrl;
            scene_manager_next_scene(app->scene_manager, Text_Input);
            consumed = true;
            break;
        case BuildHttpItemSetHeaders: // Set Headers
            scene_manager_next_scene(app->scene_manager, Build_Http_Headers);
            consumed = true;
            break;
        case BuildHttpItemSetPayload: // Set Payload
            app->selected_tx_string = furi_string_get_cstr(app->build_http_state->payload);
            app->text_input_state = TextInputState_BuildHttpPayload;
            scene_manager_next_scene(app->scene_manager, Text_Input);
            consumed = true;
            break;
        case BuildHttpItemAction: // Send Request
            // Implement the logic to send the HTTP request
            consumed = true;
            break;

        case BuildHttpItemSaveToCsv:
            FURI_LOG_T(TAG, "Saving to CSV");
            {
                // Allocate on heap instead of stack
                BuildHttpList* build_http_entry = malloc(sizeof(BuildHttpList));
                if(!build_http_entry) {
                    FURI_LOG_E(TAG, "Failed to allocate memory for CSV entry");
                    break;
                }
                memset(build_http_entry, 0, sizeof(BuildHttpList));

                // Copy URL with bounds checking
                strncpy(build_http_entry->url, app->build_http_state->url, TEXT_STORE_SIZE - 1);
                build_http_entry->url[TEXT_STORE_SIZE - 1] = '\0';

                // Copy simple fields
                build_http_entry->mode = app->build_http_state->mode;
                build_http_entry->http_method = app->build_http_state->http_method;
                build_http_entry->show_response_headers =
                    app->build_http_state->show_response_headers;

                // Handle headers
                bool has_headers = false;
                for(int i = 0; i < MAX_HEADERS && app->build_http_state->headers[i].key[0]; i++) {
                    strncpy(
                        build_http_entry->headers[i].key,
                        app->build_http_state->headers[i].key,
                        TEXT_STORE_SIZE - 1);
                    build_http_entry->headers[i].key[TEXT_STORE_SIZE - 1] = '\0';

                    strncpy(
                        build_http_entry->headers[i].value,
                        app->build_http_state->headers[i].value,
                        TEXT_STORE_SIZE - 1);
                    build_http_entry->headers[i].value[TEXT_STORE_SIZE - 1] = '\0';

                    has_headers = true;
                }

                // Handle payload safely
                if(app->build_http_state->payload &&
                   furi_string_size(app->build_http_state->payload) > 0) {
                    build_http_entry->payload =
                        furi_string_alloc_set(app->build_http_state->payload);
                } else {
                    build_http_entry->payload = furi_string_alloc();
                }

                FURI_LOG_T(TAG, "Writing to CSV");
                FURI_LOG_D(TAG, "ITEM URL: %s", build_http_entry->url);

                // Write to CSV
                bool write_success = write_build_http_to_csv(app, build_http_entry, has_headers);

                // Cleanup
                if(build_http_entry->payload) {
                    furi_string_free(build_http_entry->payload);
                }
                free(build_http_entry);

                if(!write_success) {
                    FURI_LOG_E(TAG, "Failed to write HTTP call to CSV");
                }

                sync_csv_build_http_to_mem(app);

                draw_build_http_call_menu(app);
                consumed = true;
            }
            break;
        case BuildHttpItemLoadFromCsv: // Load from CSV
            if(strlen(app->build_http_list[0].url) > 0) {
                scene_manager_next_scene(app->scene_manager, Build_Http_Url_List);
                consumed = true;
            }
            break;
        default:
            consumed = false;
            break;
        }
    }

    return consumed;
}

void scene_on_exit_build_http_call(void* context) {
    App* app = context;
    furi_assert(app);
    variable_item_list_reset(app->variable_item_list);
}
