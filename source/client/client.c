#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "service.h"

int connect_server();
 
int main(int argc , char *argv[])
{
    int sock, read_size;
    int keep_playing = 1;
    char message[1000] , server_reply[2000];
     
    sock = connect_server();
    memset(message, 0, sizeof message);

    // keep communicating with server
    while(keep_playing == 1)
    {
        // read from the server
        read_size = read(sock, server_reply, sizeof server_reply);
        if (read_size < 0)
        {
            puts("read failed");
        }
        if (strcmp(server_reply, "-1") == 0)
        {
            puts("Thanks for playing :)\n");
            keep_playing = 0;
        }
        else
        {
            // write to the user
            write(STDOUT_FILENO, server_reply, read_size);
            // write user answer to the server
            scanf("%s", message);
            write(sock, message, strlen(message));
        }
    }
    close(sock);
    puts("Server socket closed\n");
    return 0;
}