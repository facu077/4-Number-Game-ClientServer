#ifndef _SERVICE_H_
#define _SERVICE_H_

typedef struct guess
{
    int regular;
    int good;
    char number[4];
} Guess;

int start_server();
char * writeAndRead(int socket, char * message);

#endif