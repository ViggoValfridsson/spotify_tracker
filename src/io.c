#include "io.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int get_input(const char *prompt, size_t max_len, char **input_out) {
    int return_value = STATUS_ERROR;

    if (max_len == 0)
        return STATUS_ERROR;

    printf("%s", prompt);
    fflush(stdout);

    char *input = malloc(max_len + 1);
    if (!input) {
        perror("malloc");
        goto cleanup;
    }

    if (fgets(input, max_len + 1, stdin) == NULL) {
        perror("fgets");
        goto cleanup;
    }

    // Trim trailing newline
    size_t len = strlen(input);
    if (len > 0 && input[len - 1] == '\n') {
        input[len - 1] = '\0';
        len--;
    }

    *input_out = input;
    input = NULL;
    return_value = STATUS_SUCCESS;

cleanup:
    free(input);
    return return_value;
}
