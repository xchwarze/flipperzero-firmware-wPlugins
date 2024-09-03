#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <furi/core/record.h>
#include <furi/core/string.h>
#include <lib/datetime/datetime.h>
#include <storage/filesystem_api_defines.h>
#include <storage/storage.h>
#include <targets/f7/furi_hal/furi_hal_rtc.h>
#include <toolbox/stream/file_stream.h>
#include <toolbox/stream/stream.h>

typedef struct FileLines {
    const char** lines;
    size_t num_lines;
} FileLines;

typedef struct InsultsFiles {
    Storage* storage;
    FileLines* imperatives;
    FileLines* adjectives;
    FileLines* nouns;
    FileLines* overlaps;
} InsultsFiles;

FileLines* file_lines_alloc(void);

void file_lines_free(FileLines* file_lines);

InsultsFiles* insults_files_alloc(void);

void insults_files_free(InsultsFiles* insults_files);

void insults_files_get_lines(InsultsFiles* insults_files, const char* path, FileLines* file_lines);

bool insults_files_check_overlap(
    InsultsFiles* insults_files,
    const char* adjective,
    const char* noun);

char* insults_files_get_insult(InsultsFiles* insults_files);

void insults_files_save_insult(InsultsFiles* insults_files, const char* insult);

void insults_files_delete_insult(InsultsFiles* insults_files, const unsigned int index);

void insults_files_clear_insults(InsultsFiles* insults_files);
