#ifndef FLIP_WEATHER_CALLBACK_H
#define FLIP_WEATHER_CALLBACK_H

static bool sent_get_request = false;
static bool get_request_success = false;
static bool weather_request_success = false;
static bool got_ip_address = false;
static bool sent_weather_request = false;
static bool got_weather_data = false;
static bool geo_information_processed = false;
static bool weather_information_processed = false;
static char ip_address[16];
static char city_data[48];
static char region_data[48];
static char country_data[48];
static char lat_data[32];
static char lon_data[32];
static char ip_data[32];
static char temperature_data[32];
static char precipitation_data[32];
static char rain_data[32];
static char showers_data[32];
static char snowfall_data[32];
static char time_data[32];

#define MAX_TOKENS 64 // Adjust based on expected JSON size (50)

// Helper function to compare JSON keys
int jsoneq(const char* json, jsmntok_t* tok, const char* s) {
    if(tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
       strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

// return the value of the key in the JSON data
// works for the first level of the JSON data
char* get_json_value(char* key, char* json_data, uint32_t max_tokens) {
    // Parse the JSON feed
    if(json_data != NULL) {
        jsmn_parser parser;
        jsmn_init(&parser);

        // Allocate tokens array on the heap
        jsmntok_t* tokens = malloc(sizeof(jsmntok_t) * max_tokens);
        if(tokens == NULL) {
            FURI_LOG_E(TAG, "Failed to allocate memory for JSON tokens.");
            return NULL;
        }

        int ret = jsmn_parse(&parser, json_data, strlen(json_data), tokens, max_tokens);
        if(ret < 0) {
            // Handle parsing errors
            FURI_LOG_E(TAG, "Failed to parse JSON: %d", ret);
            free(tokens);
            return NULL;
        }

        // Ensure that the root element is an object
        if(ret < 1 || tokens[0].type != JSMN_OBJECT) {
            FURI_LOG_E(TAG, "Root element is not an object.");
            free(tokens);
            return NULL;
        }

        // Loop through the tokens to find the key
        for(int i = 1; i < ret; i++) {
            if(jsoneq(json_data, &tokens[i], key) == 0) {
                // We found the key. Now, return the associated value.
                int length = tokens[i + 1].end - tokens[i + 1].start;
                char* value = malloc(length + 1);
                if(value == NULL) {
                    FURI_LOG_E(TAG, "Failed to allocate memory for value.");
                    free(tokens);
                    return NULL;
                }
                strncpy(value, json_data + tokens[i + 1].start, length);
                value[length] = '\0'; // Null-terminate the string

                free(tokens); // Free the token array
                return value; // Return the extracted value
            }
        }

        // Free the token array if key was not found
        free(tokens);
    } else {
        FURI_LOG_E(TAG, "JSON data is NULL");
    }
    FURI_LOG_E(TAG, "Failed to find the key in the JSON.");
    return NULL; // Return NULL if something goes wrong
}

void flip_weather_request_error(Canvas* canvas) {
    if(fhttp.received_data == NULL) {
        if(fhttp.last_response != NULL) {
            if(strstr(fhttp.last_response, "[ERROR] Not connected to Wifi. Failed to reconnect.") !=
               NULL) {
                canvas_clear(canvas);
                canvas_draw_str(canvas, 0, 10, "[ERROR] Not connected to Wifi.");
                canvas_draw_str(canvas, 0, 50, "Update your WiFi settings.");
                canvas_draw_str(canvas, 0, 60, "Press BACK to return.");
            } else if(strstr(fhttp.last_response, "[ERROR] Failed to connect to Wifi.") != NULL) {
                canvas_clear(canvas);
                canvas_draw_str(canvas, 0, 10, "[ERROR] Not connected to Wifi.");
                canvas_draw_str(canvas, 0, 50, "Update your WiFi settings.");
                canvas_draw_str(canvas, 0, 60, "Press BACK to return.");
            } else {
                canvas_clear(canvas);
                FURI_LOG_E(TAG, "Received an error: %s", fhttp.last_response);
                canvas_draw_str(canvas, 0, 10, "[ERROR] Unusual error...");
                canvas_draw_str(canvas, 0, 60, "Press BACK and retry.");
            }
        } else {
            canvas_clear(canvas);
            canvas_draw_str(canvas, 0, 10, "[ERROR] Unknown error.");
            canvas_draw_str(canvas, 0, 50, "Update your WiFi settings.");
            canvas_draw_str(canvas, 0, 60, "Press BACK to return.");
        }
    } else {
        canvas_clear(canvas);
        canvas_draw_str(canvas, 0, 10, "Failed to receive data.");
        canvas_draw_str(canvas, 0, 60, "Press BACK to return.");
    }
}

static bool send_geo_location_request() {
    if(!sent_get_request && fhttp.state == IDLE) {
        sent_get_request = true;
        char payload[256] = {0};
        snprintf(payload, 256, "{\"ip\": \"%s\"}", ip_address);
        get_request_success = flipper_http_post_request_with_headers(
            "https://www.flipsocial.net/api/geo-location/",
            "{\"Content-Type\": \"application/json\"}",
            payload);
        if(!get_request_success) {
            FURI_LOG_E(TAG, "Failed to send GET request");
            return false;
        }
        fhttp.state = RECEIVING;
    }
    return true;
}

static void process_geo_location() {
    if(!geo_information_processed && fhttp.received_data != NULL) {
        geo_information_processed = true;
        char* city = get_json_value("city", fhttp.received_data, MAX_TOKENS);
        char* region = get_json_value("region", fhttp.received_data, MAX_TOKENS);
        char* country = get_json_value("country", fhttp.received_data, MAX_TOKENS);
        char* latitude = get_json_value("latitude", fhttp.received_data, MAX_TOKENS);
        char* longitude = get_json_value("longitude", fhttp.received_data, MAX_TOKENS);

        snprintf(city_data, 64, "City: %s", city);
        snprintf(region_data, 64, "Region: %s", region);
        snprintf(country_data, 64, "Country: %s", country);
        snprintf(lat_data, 64, "Latitude: %s", latitude);
        snprintf(lon_data, 64, "Longitude: %s", longitude);
        snprintf(ip_data, 64, "IP Address: %s", ip_address);

        fhttp.state = IDLE;
    }
}

static void process_weather() {
    if(!weather_information_processed && fhttp.received_data != NULL) {
        weather_information_processed = true;
        char* current_data = get_json_value("current", fhttp.received_data, MAX_TOKENS);
        char* temperature = get_json_value("temperature_2m", current_data, MAX_TOKENS);
        char* precipitation = get_json_value("precipitation", current_data, MAX_TOKENS);
        char* rain = get_json_value("rain", current_data, MAX_TOKENS);
        char* showers = get_json_value("showers", current_data, MAX_TOKENS);
        char* snowfall = get_json_value("snowfall", current_data, MAX_TOKENS);
        char* time = get_json_value("time", current_data, MAX_TOKENS);

        // replace the "T" in time with a space
        char* ptr = strstr(time, "T");
        if(ptr != NULL) {
            *ptr = ' ';
        }

        snprintf(temperature_data, 64, "Temperature (C): %s", temperature);
        snprintf(precipitation_data, 64, "Precipitation: %s", precipitation);
        snprintf(rain_data, 64, "Rain: %s", rain);
        snprintf(showers_data, 64, "Showers: %s", showers);
        snprintf(snowfall_data, 64, "Snowfall: %s", snowfall);
        snprintf(time_data, 64, "Time: %s", time);

        fhttp.state = IDLE;
    }
}

static void flip_weather_handle_gps_draw(Canvas* canvas, bool show_gps_data) {
    if(sent_get_request) {
        if(fhttp.state == RECEIVING) {
            if(show_gps_data) {
                canvas_clear(canvas);
                canvas_draw_str(canvas, 0, 10, "Loading GPS...");
                canvas_draw_str(canvas, 0, 22, "Receiving...");
            }
        }
        // check status
        else if(fhttp.state == ISSUE || !get_request_success || fhttp.received_data == NULL) {
            flip_weather_request_error(canvas);
        } else if(fhttp.state == IDLE && fhttp.received_data != NULL) {
            // success, draw GPS
            process_geo_location();

            if(show_gps_data) {
                canvas_clear(canvas);
                canvas_draw_str(canvas, 0, 10, city_data);
                canvas_draw_str(canvas, 0, 20, region_data);
                canvas_draw_str(canvas, 0, 30, country_data);
                canvas_draw_str(canvas, 0, 40, lat_data);
                canvas_draw_str(canvas, 0, 50, lon_data);
                canvas_draw_str(canvas, 0, 60, ip_data);
            }
        }
    }
}

// Callback for drawing the weather screen
static void flip_weather_view_draw_callback_weather(Canvas* canvas, void* model) {
    if(!canvas) {
        return;
    }
    UNUSED(model);

    canvas_set_font(canvas, FontSecondary);

    if(fhttp.state == INACTIVE) {
        canvas_draw_str(canvas, 0, 7, "Wifi Dev Board disconnected.");
        canvas_draw_str(canvas, 0, 17, "Please connect to the board.");
        canvas_draw_str(canvas, 0, 32, "If your board is connected,");
        canvas_draw_str(canvas, 0, 42, "make sure you have flashed");
        canvas_draw_str(canvas, 0, 52, "your WiFi Devboard with the");
        canvas_draw_str(canvas, 0, 62, "latest FlipperHTTP flash.");
        return;
    }

    canvas_draw_str(canvas, 0, 10, "Loading Weather...");
    // handle geo location until it's processed and then handle weather

    // start the process
    if(!send_geo_location_request()) {
        flip_weather_request_error(canvas);
    }
    // wait until geo location is processed
    if(!sent_get_request || !get_request_success || fhttp.state == RECEIVING) {
        return;
    }
    // get/set geo lcoation once
    if(!geo_information_processed) {
        flip_weather_handle_gps_draw(canvas, false);
    }
    // start the weather process
    if(!sent_weather_request && fhttp.state == IDLE) {
        sent_weather_request = true;
        char url[256];
        char* lattitude = lat_data + 10;
        char* longitude = lon_data + 11;
        snprintf(
            url,
            256,
            "https://api.open-meteo.com/v1/forecast?latitude=%s&longitude=%s&current=temperature_2m,precipitation,rain,showers,snowfall&temperature_unit=celsius&wind_speed_unit=mph&precipitation_unit=inch&forecast_days=1",
            lattitude,
            longitude);
        weather_request_success =
            flipper_http_get_request_with_headers(url, "{\"Content-Type\": \"application/json\"}");
        if(!weather_request_success) {
            FURI_LOG_E(TAG, "Failed to send GET request");
            flip_weather_request_error(canvas);
        }
        fhttp.state = RECEIVING;
    } else {
        if(fhttp.state == RECEIVING) {
            canvas_draw_str(canvas, 0, 10, "Loading Weather...");
            canvas_draw_str(canvas, 0, 22, "Receiving...");
            return;
        }
        // check status
        else if(fhttp.state == ISSUE || !weather_request_success || fhttp.received_data == NULL) {
            flip_weather_request_error(canvas);
        } else {
            // success, draw weather
            process_weather();
            canvas_clear(canvas);
            canvas_draw_str(canvas, 0, 10, temperature_data);
            canvas_draw_str(canvas, 0, 20, precipitation_data);
            canvas_draw_str(canvas, 0, 30, rain_data);
            canvas_draw_str(canvas, 0, 40, showers_data);
            canvas_draw_str(canvas, 0, 50, snowfall_data);
            canvas_draw_str(canvas, 0, 60, time_data);
        }
    }
}

// Callback for drawing the GPS screen
static void flip_weather_view_draw_callback_gps(Canvas* canvas, void* model) {
    if(!canvas) {
        return;
    }
    UNUSED(model);

    if(fhttp.state == INACTIVE) {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 0, 7, "Wifi Dev Board disconnected.");
        canvas_draw_str(canvas, 0, 17, "Please connect to the board.");
        canvas_draw_str(canvas, 0, 32, "If your board is connected,");
        canvas_draw_str(canvas, 0, 42, "make sure you have flashed");
        canvas_draw_str(canvas, 0, 52, "your WiFi Devboard with the");
        canvas_draw_str(canvas, 0, 62, "latest FlipperHTTP flash.");
        return;
    }

    if(!send_geo_location_request()) {
        flip_weather_request_error(canvas);
    }

    flip_weather_handle_gps_draw(canvas, true);
}

// Input callback for the view (async input handling)
bool flip_weather_view_input_callback_weather(InputEvent* event, void* context) {
    FlipWeatherApp* app = (FlipWeatherApp*)context;
    UNUSED(app);
    if(event->type == InputTypePress && event->key == InputKeyBack) {
        // Exit the app when the back button is pressed
        // view_dispatcher_stop(app->view_dispatcher);
        // return true;
    }
    return false;
}

// Input callback for the view (async input handling)
bool flip_weather_view_input_callback_gps(InputEvent* event, void* context) {
    FlipWeatherApp* app = (FlipWeatherApp*)context;
    UNUSED(app);
    if(event->type == InputTypePress && event->key == InputKeyBack) {
        // Exit the app when the back button is pressed
        // view_dispatcher_stop(app->view_dispatcher);
        // return true;
    }
    return false;
}
// handle the async-to-sync process to get and set the IP address
static bool flip_weather_handle_ip_address() {
    if(!got_ip_address) {
        got_ip_address = true;
        if(!flipper_http_get_request("https://httpbin.org/get")) {
            FURI_LOG_E(TAG, "Failed to get IP address");
            return false;
        } else {
            fhttp.state = RECEIVING;
            furi_timer_start(fhttp.get_timeout_timer, TIMEOUT_DURATION_TICKS);
        }
        while(fhttp.state == RECEIVING && furi_timer_is_running(fhttp.get_timeout_timer) > 0) {
            // Wait for the feed to be received
            furi_delay_ms(100);
        }
        furi_timer_stop(fhttp.get_timeout_timer);
        ip_address[0] = '\0';
        if(fhttp.received_data != NULL) {
            char* ip = get_json_value("origin", fhttp.received_data, MAX_TOKENS);
            if(ip == NULL) {
                FURI_LOG_E(TAG, "Failed to get IP address");
                sent_get_request = true;
                get_request_success = false;
                fhttp.state = ISSUE;
                return false;
            }
            strncpy(ip_address, ip, 15);
            ip_address[15] = '\0';
        } else {
            FURI_LOG_E(TAG, "Failed to get IP address");
            sent_get_request = true;
            get_request_success = false;
            fhttp.state = ISSUE;
            return false;
        }
    }
    return true;
}

static void callback_submenu_choices(void* context, uint32_t index) {
    FlipWeatherApp* app = (FlipWeatherApp*)context;
    if(!app) {
        FURI_LOG_E(TAG, "FlipWeatherApp is NULL");
        return;
    }
    switch(index) {
    case FlipWeatherSubmenuIndexWeather:
        if(!flip_weather_handle_ip_address()) {
            return;
        }
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipWeatherViewWeather);
        break;
    case FlipWeatherSubmenuIndexGPS:
        if(!flip_weather_handle_ip_address()) {
            return;
        }
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipWeatherViewGPS);
        break;
    case FlipWeatherSubmenuIndexAbout:
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipWeatherViewAbout);
        break;
    case FlipWeatherSubmenuIndexSettings:
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipWeatherViewSettings);
        break;
    default:
        break;
    }
}

static void text_updated_ssid(void* context) {
    FlipWeatherApp* app = (FlipWeatherApp*)context;
    if(!app) {
        FURI_LOG_E(TAG, "FlipWeatherApp is NULL");
        return;
    }

    // store the entered text
    strncpy(
        app->uart_text_input_buffer_ssid,
        app->uart_text_input_temp_buffer_ssid,
        app->uart_text_input_buffer_size_ssid);

    // Ensure null-termination
    app->uart_text_input_buffer_ssid[app->uart_text_input_buffer_size_ssid - 1] = '\0';

    // update the variable item text
    if(app->variable_item_ssid) {
        variable_item_set_current_value_text(
            app->variable_item_ssid, app->uart_text_input_buffer_ssid);
    }

    // save settings
    save_settings(app->uart_text_input_buffer_ssid, app->uart_text_input_buffer_password);

    // save wifi settings to devboard
    if(strlen(app->uart_text_input_buffer_ssid) > 0 &&
       strlen(app->uart_text_input_buffer_password) > 0) {
        if(!flipper_http_save_wifi(
               app->uart_text_input_buffer_ssid, app->uart_text_input_buffer_password)) {
            FURI_LOG_E(TAG, "Failed to save wifi settings");
        }
    }

    // switch to the settings view
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipWeatherViewSettings);
}

static void text_updated_password(void* context) {
    FlipWeatherApp* app = (FlipWeatherApp*)context;
    if(!app) {
        FURI_LOG_E(TAG, "FlipWeatherApp is NULL");
        return;
    }

    // store the entered text
    strncpy(
        app->uart_text_input_buffer_password,
        app->uart_text_input_temp_buffer_password,
        app->uart_text_input_buffer_size_password);

    // Ensure null-termination
    app->uart_text_input_buffer_password[app->uart_text_input_buffer_size_password - 1] = '\0';

    // update the variable item text
    if(app->variable_item_password) {
        variable_item_set_current_value_text(
            app->variable_item_password, app->uart_text_input_buffer_password);
    }

    // save settings
    save_settings(app->uart_text_input_buffer_ssid, app->uart_text_input_buffer_password);

    // save wifi settings to devboard
    if(strlen(app->uart_text_input_buffer_ssid) > 0 &&
       strlen(app->uart_text_input_buffer_password) > 0) {
        if(!flipper_http_save_wifi(
               app->uart_text_input_buffer_ssid, app->uart_text_input_buffer_password)) {
            FURI_LOG_E(TAG, "Failed to save wifi settings");
        }
    }

    // switch to the settings view
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipWeatherViewSettings);
}

static uint32_t callback_to_submenu(void* context) {
    if(!context) {
        FURI_LOG_E(TAG, "Context is NULL");
        return VIEW_NONE;
    }
    UNUSED(context);
    sent_get_request = false;
    get_request_success = false;
    got_ip_address = false;
    got_weather_data = false;
    geo_information_processed = false;
    weather_information_processed = false;
    sent_weather_request = false;
    weather_request_success = false;
    return FlipWeatherViewSubmenu;
}

static void settings_item_selected(void* context, uint32_t index) {
    FlipWeatherApp* app = (FlipWeatherApp*)context;
    if(!app) {
        FURI_LOG_E(TAG, "FlipWeatherApp is NULL");
        return;
    }
    switch(index) {
    case 0: // Input SSID
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipWeatherViewTextInputSSID);
        break;
    case 1: // Input Password
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipWeatherViewTextInputPassword);
        break;
    default:
        FURI_LOG_E(TAG, "Unknown configuration item index");
        break;
    }
}

/**
 * @brief Navigation callback for exiting the application
 * @param context The context - unused
 * @return next view id (VIEW_NONE to exit the app)
 */
static uint32_t callback_exit_app(void* context) {
    // Exit the application
    if(!context) {
        FURI_LOG_E(TAG, "Context is NULL");
        return VIEW_NONE;
    }
    UNUSED(context);
    return VIEW_NONE; // Return VIEW_NONE to exit the app
}

#endif
