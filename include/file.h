#ifndef FILE_H
#define FILE_H

#include "common.h"

status_code read_file_content(const char *file_path, char **file_content_out);
status_code write_file_overwrite(const char *file_path, const char *file_content);

#endif

