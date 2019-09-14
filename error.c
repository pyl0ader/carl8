#include "error.h"

char error[ERROR_BUFFER_SIZE] = {};

#include <stdio.h>
#include <string.h>

void logError(const char* context, const char* errorAdd)
{
    char tmp[ERROR_BUFFER_SIZE] = {};

    strcpy(tmp, errorAdd);
    snprintf(error, ERROR_BUFFER_SIZE, "%s: %s", context, tmp);
    return;
}

char* getError(void)
{
    return error;
}
