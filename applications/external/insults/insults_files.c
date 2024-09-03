#include "insults_files.h"

FileLines* file_lines_alloc(void) {
    FileLines* file_lines = (FileLines*)malloc(sizeof(FileLines));

    file_lines->lines = (const char**)malloc(sizeof(const char*));

    return file_lines;
}

void file_lines_free(FileLines* file_lines) {
    free(file_lines->lines);
    free(file_lines);
}

InsultsFiles* insults_files_alloc(void) {
    DateTime date_time;

    furi_hal_rtc_get_datetime(&date_time);

    srand(datetime_datetime_to_timestamp(&date_time));

    InsultsFiles* insults_files = (InsultsFiles*)malloc(sizeof(InsultsFiles));

    insults_files->storage = furi_record_open(RECORD_STORAGE);
    insults_files->imperatives = file_lines_alloc();
    insults_files->adjectives = file_lines_alloc();
    insults_files->nouns = file_lines_alloc();
    insults_files->overlaps = file_lines_alloc();

    insults_files_get_lines(
        insults_files, APP_ASSETS_PATH("word_lists/imperatives.txt"), insults_files->imperatives);
    insults_files_get_lines(
        insults_files, APP_ASSETS_PATH("word_lists/adjectives.txt"), insults_files->adjectives);
    insults_files_get_lines(
        insults_files, APP_ASSETS_PATH("word_lists/nouns.txt"), insults_files->nouns);
    insults_files_get_lines(
        insults_files, APP_ASSETS_PATH("word_lists/overlaps.txt"), insults_files->overlaps);

    return insults_files;
}

void insults_files_free(InsultsFiles* insults_files) {
    file_lines_free(insults_files->imperatives);
    file_lines_free(insults_files->adjectives);
    file_lines_free(insults_files->nouns);
    file_lines_free(insults_files->overlaps);

    furi_record_close(RECORD_STORAGE);

    insults_files->storage = NULL;

    free(insults_files);
}

void insults_files_get_lines(InsultsFiles* insults_files, const char* path, FileLines* file_lines) {
    size_t num_lines = 0;
    size_t lines_size = 1;

    Stream* file = file_stream_alloc(insults_files->storage);

    if(file_stream_open(file, path, FSAM_READ, FSOM_OPEN_EXISTING)) {
        FuriString* line = furi_string_alloc();

        while(stream_read_line(file, line)) {
            furi_string_replace_all(line, "\r", "");
            furi_string_replace_all(line, "\n", "");

            if(!furi_string_empty(line)) {
                if(lines_size <= num_lines) {
                    lines_size *= 2;

                    file_lines->lines =
                        (const char**)realloc(file_lines->lines, sizeof(const char*) * lines_size);
                }

                file_lines->lines[num_lines++] = strdup(furi_string_get_cstr(line));
            }
        }

        furi_string_free(line);
    }

    file_stream_close(file);

    stream_free(file);

    file_lines->num_lines = num_lines;
}

bool insults_files_check_overlap(
    InsultsFiles* insults_files,
    const char* adjective,
    const char* noun) {
    for(unsigned int i = 0; i < insults_files->overlaps->num_lines; i++) {
        const char* line = insults_files->overlaps->lines[i];

        const char* adjective_temp = adjective;
        const char* noun_temp = noun;

        while(*adjective_temp != '\0' && *line != '\0' && *adjective_temp == *line) {
            adjective_temp++;
            line++;
        }

        if(*adjective_temp != '\0' || *line != ':') {
            continue;
        }

        line++;

        while(*noun_temp != '\0' && *line != '\0' && *noun_temp == *line) {
            noun_temp++;
            line++;
        }

        if(*noun_temp == '\0' && *line == '\0') {
            return true;
        }
    }

    return false;
}

char* insults_files_get_insult(InsultsFiles* insults_files) {
    const char* adjective;
    const char* noun;

    do {
        adjective =
            insults_files->adjectives->lines[rand() % insults_files->adjectives->num_lines];
        noun = insults_files->nouns->lines[rand() % insults_files->nouns->num_lines];
    } while(insults_files_check_overlap(insults_files, adjective, noun));

    FuriString* insult_string = furi_string_alloc_printf(
        "%s, %s %s!",
        insults_files->imperatives->lines[rand() % insults_files->imperatives->num_lines],
        adjective,
        noun);

    char* insult_cstr = strdup(furi_string_get_cstr(insult_string));

    furi_string_free(insult_string);

    return insult_cstr;
}

void insults_files_save_insult(InsultsFiles* insults_files, const char* insult) {
    File* file = storage_file_alloc(insults_files->storage);

    if(storage_file_open(
           file, APP_DATA_PATH("saved_insults.txt"), FSAM_READ_WRITE, FSOM_OPEN_APPEND)) {
        storage_file_write(file, insult, strlen(insult));
        storage_file_write(file, "\n", 1);
    }

    storage_file_close(file);
    storage_file_free(file);
}

void insults_files_delete_insult(InsultsFiles* insults_files, const unsigned int index) {
    FileLines* lines = file_lines_alloc();

    insults_files_get_lines(insults_files, APP_DATA_PATH("saved_insults.txt"), lines);

    File* file = storage_file_alloc(insults_files->storage);

    if(storage_file_open(
           file, APP_DATA_PATH("saved_insults.txt"), FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        for(unsigned int i = 0; i < lines->num_lines; i++) {
            if(i != index) {
                storage_file_write(file, lines->lines[i], strlen(lines->lines[i]));
                storage_file_write(file, "\n", 1);
            }
        }
    }

    storage_file_close(file);
    storage_file_free(file);

    file_lines_free(lines);
}

void insults_files_clear_insults(InsultsFiles* insults_files) {
    File* file = storage_file_alloc(insults_files->storage);

    storage_file_open(file, APP_DATA_PATH("saved_insults.txt"), FSAM_WRITE, FSOM_CREATE_ALWAYS);
    storage_file_close(file);
    storage_file_free(file);
}
