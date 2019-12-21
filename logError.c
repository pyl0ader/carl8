#include "logError.h"

static char errorLog[ERROR_LOG_SIZE][ERROR_BUFFER_SIZE] = {};
static int errorIndex = 0;

static void dump(void);

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

static FILE* dumpFile;

void setError(const char *fmt, ...)
{
    char tmp[ERROR_BUFFER_SIZE]; 
    va_list ap;

    va_start(ap, fmt);
    
    vsnprintf(tmp, ERROR_BUFFER_SIZE, fmt, ap);
    strcpy(errorLog[errorIndex], tmp);
    return;
}

char* getError(void)
{
    return errorLog[errorIndex];
}

void logError(void){
    errorIndex++;
    if(errorIndex == ERROR_LOG_SIZE)
        dump();
    return;
}

void presentErrorLog(void){

    char *buffer = malloc(ERROR_BUFFER_SIZE);
    size_t n = ERROR_BUFFER_SIZE;

    if(dumpFile != NULL){
        fseek(dumpFile, 0, SEEK_SET);
        while(getline(&buffer, &n, dumpFile) > 0 )
            fputs(buffer, stderr);
    }
    for(int i = 0; i <= errorIndex; i++){
        fprintf(stderr, "%s\n", errorLog[i]);
    }

    free(buffer);
}

void dump(void){

    if(dumpFile == NULL){
        dumpFile = tmpfile();
    }

    for(int i = 0; i < errorIndex; i++){
        fprintf(dumpFile, "%s\n", errorLog[i]);
    }

    errorIndex = 0;
}
