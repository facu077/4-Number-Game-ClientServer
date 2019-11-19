#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "service.h"
 
int main(int argc , char *argv[])
{
    int sock, read_size;
    char message[1000], server_reply[2000];

    if (argc != 3)
    {
        printf("Usage: ./client <server_ip> <server_port>\n");
        return 0;
    }
    sock = connect_server(argv[1], argv[2]);
    memset(message, 0, sizeof message);

    // keep communicating with server
    while((read_size = read(sock, server_reply, sizeof(server_reply))) > 0)
    {
        if (read_size < 0)
        {
            puts("read failed");
        }
        if (strcmp(server_reply, "-1") == 0)
        {
            puts("Thanks for playing :)\n");
            close(sock);
            puts("Server socket closed\n");
        }
        else if(read_size > 0)
        {
            // write to the user
            write(STDOUT_FILENO, server_reply, read_size);
            // write user answer to the server
            scanf("%s", message);
            write(sock, message, strlen(message));
        }
    }
    return 0;
}