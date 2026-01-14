#ifndef FILE_H
#define FILE_H

int read_file_content(const char *file_path, char **file_content_out);
int write_file_overwrite(const char *file_path, const char *file_content);

#endif

