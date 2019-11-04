#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include "service.h"

int connect_server(char * server_ip)
{
    char *hostname = server_ip;
    char *service = "8888";
    struct addrinfo hints, *res;
    int err;
    int sock;

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = PF_UNSPEC;


    if ((err = getaddrinfo(hostname, service, &hints, &res)) != 0)
    {
        printf("error %d : %s\n", err, gai_strerror(err));
        return -1;
    }

    sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0)
    {
        perror("create socket failed");
        return -1;
    }
    puts("Socket created");

    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0)
    {
        perror("connect failed");
        return 1;
    }

    freeaddrinfo(res);

    puts("Connected");

    return sock;
}