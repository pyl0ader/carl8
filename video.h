int initializeVideo(const char* name);
int videoProcess(unsigned char scrn[64 * 32]);
int clear(void);
char* videoError(void);
void closeVideo(void);

extern const unsigned short int WIDTH;
extern const unsigned short int HEIGHT;
