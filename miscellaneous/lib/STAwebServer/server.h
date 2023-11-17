#ifndef _SERVER_H
#define _SERVER_H
#ifndef LED_BUILTIN
    #define LED_BUILTIN 2
#endif
void setupServer();
void runServer(const char*);
#endif