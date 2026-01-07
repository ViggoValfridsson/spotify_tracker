#include "file.h"
#include "common.h"
#include <curl/curl.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_FILE_PATH 2048

void print_fopen_error() {
    if (errno == ENOENT)
        fprintf(stderr, "Cannot read credentials file: File doesn't exist\n");
    else if (errno == EACCES)
        fprintf(stderr, "Cannot read credentials file: Permission denied\n");
    else
        fprintf(stderr, "Cannot read credentials file: Error %d\n", errno);
}

// Expand ~ to actual home directory path
int expand_file_name(char *file_name, char **resolved_name_out) {
    char *resolved_name = malloc(MAX_FILE_PATH);

    if (!resolved_name)
        return STATUS_ERROR;

    if (file_name[0] != '~') {
        strncpy(resolved_name, file_name, MAX_FILE_PATH - 1);
    } else {
        char *home = getenv("HOME");

        if (!home) {
            fprintf(stderr, "Home directory environment variable not set\n");
            free(resolved_name);
            return STATUS_ERROR;
        }

        int result = snprintf(resolved_name, MAX_FILE_PATH, "%s%s", home, file_name + 1);

        if (result >= MAX_FILE_PATH) {
            fprintf(stderr, "Credentials file path is too long. Max path length is %d\n", MAX_FILE_PATH);
            free(resolved_name);
            return STATUS_ERROR;
        }
        if (result < 0) {
            perror("snprintf");
            free(resolved_name);
            return STATUS_ERROR;
        }
    }

    *resolved_name_out = resolved_name;
    return STATUS_SUCCESS;
}

int open_file(char *file_name, char *open_mode, FILE **file_out) {
    char *resolved_name = NULL;
    errno = 0;

    int return_value = expand_file_name(file_name, &resolved_name);
    if (return_value != STATUS_SUCCESS)
        goto cleanup;

    FILE *file = fopen(resolved_name, open_mode);
    if (!file) {
        print_fopen_error();
        return_value = STATUS_FILE_ERROR;
        goto cleanup;
    }

    *file_out = file;
    return_value = STATUS_SUCCESS;

cleanup:
    free(resolved_name);
    return return_value;
}

int get_file_size(FILE *file, long *size_out) {
    if (fseek(file, 0, SEEK_END) == -1) {
        perror("fseek");
        return STATUS_FILE_ERROR;
    }

    long size = ftell(file);
    if (size == -1) {
        perror("ftell");
        return STATUS_FILE_ERROR;
    }

    if (fseek(file, 0, SEEK_SET) == -1) {
        perror("fseek");
        return STATUS_FILE_ERROR;
    }

    *size_out = size;
    return STATUS_SUCCESS;
}

int read_file(FILE *file, long size, char **data_out) {
    char *data = malloc(size + 1);

    if (data == NULL) {
        perror("malloc");
        return STATUS_ERROR;
    }

    if (fread(data, 1, size, file) == 0 && ferror(file) != 0) {
        free(data);
        perror("fread");
        return STATUS_FILE_ERROR;
    }

    data[size] = '\0';
    *data_out = data;

    return STATUS_SUCCESS;
}

int read_file_content(char *file_path, char **file_content_out) {
    FILE *file = NULL;

    int return_value = open_file(file_path, "r", &file);
    if (return_value != STATUS_SUCCESS)
        goto cleanup;

    long size;
    return_value = get_file_size(file, &size);
    if (return_value != STATUS_SUCCESS)
        goto cleanup;

    char *data;
    return_value = read_file(file, size, &data);
    if (return_value != STATUS_SUCCESS)
        goto cleanup;

    *file_content_out = data;
    return_value = STATUS_SUCCESS;

cleanup:
    if (file)
        fclose(file);

    return STATUS_SUCCESS;
}

int write_file_overwrite(char *file_path, char *file_content) {
    FILE *file = NULL;

    if (open_file(file_path, "w", &file) != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to open refresh token file");
        return STATUS_FILE_ERROR;
    }

    if (fprintf(file, "%s", file_content) < 0) {
        fprintf(stderr, "Failed to write to refresh token file");
        fclose(file);
        return STATUS_ERROR;
    }

    fclose(file);
    return STATUS_SUCCESS;
}
