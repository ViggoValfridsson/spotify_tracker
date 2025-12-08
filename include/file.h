#ifndef FILE_H
#define FILE_H

#include "network.h"

int read_file_content(char *file_path, char **file_content);
int read_credentials_from_file(char *credentials_file_path, client_credentials **credentials_out);

#endif

