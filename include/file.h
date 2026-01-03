#ifndef FILE_H
#define FILE_H

int read_file_content(char *file_path, char **file_content_out);
int write_file_overwrite(char *file_path, char *file_content);

#endif

