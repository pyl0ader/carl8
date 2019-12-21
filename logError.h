#define ERROR_BUFFER_SIZE 120
#define ERROR_LOG_SIZE 80

void setError(const char* fmt, ...);
char* getError(void);
void logError(void);
void presentErrorLog(void);
