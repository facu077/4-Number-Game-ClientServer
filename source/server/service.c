#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "service.h"

// SERVER

int start_server()
{
    int socket_desc;
    struct sockaddr_in server;

    // Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    // Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );
     
    // Bind
    if (bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("bind failed");
        return 1;
    }
    puts("bind done");
     
    // Listen
    listen(socket_desc , 3);
     
    puts("Waiting for incoming connections...");

    return socket_desc;
}

char * writeAndRead(int socket, char * message)
{
    int read_size;
    // char answer[2000];
    char* answer = (char*)malloc(sizeof(char)*(2000));
    // Write to client
    write(socket, message, strlen(message));
    // Read answer from client
    read_size = read(socket, answer, sizeof(answer));
    if(read_size == -1)
    {
        perror("read failed");
    }
    // TODO Free answer (how?)
    return answer;
}

// THREADS