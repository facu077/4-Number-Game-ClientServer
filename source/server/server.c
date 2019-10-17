#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "service.h"

int main(int argc , char *argv[])
{
    int socket_desc, client_sock, c;
    struct sockaddr_in client;
    char client_message[2000];
    char server_message[2000];
    int keep_playing = 1;
    char *ip;
    pid_t pid;

    socket_desc = start_server();
    c = sizeof(struct sockaddr_in);

    //Accept connection from an incoming client
    while ((client_sock = accept(socket_desc,(struct sockaddr *)&client,(socklen_t*)&c))>0)
    {
        puts("Connection accepted");
        ip = inet_ntoa(client.sin_addr);
        printf("Client IP: %s and PORT: %d\n",ip, ntohs(client.sin_port));
        pid = fork();
        if (pid == 0)
        {
            // Ask client name
            strcpy(client_message, writeAndRead(client_sock, "Please enter your name: "));
            // TODO Look in the log.txt file for the user data
            strcpy(server_message, "Welcome ");
            strcat(server_message, client_message);
            strcat(server_message, ", you have played 8 times, with an average of 4 attempts per correct answer\n");
            strcat(server_message, "Is your number 1234?\n");
            // Set welcome message to client with a random number
            while(keep_playing == 1)
            {
                // Ask the number to the client
                strcpy(client_message, writeAndRead(client_sock, server_message));
                // Read user input
                if (strcmp(client_message, "yes") == 0)
                {
                    // GAME OVER
                    // TODO save result in log.txt
                    write(client_sock, "-1", 3);
                    keep_playing = 0;
                }
                else
                {
                    // Keep playing
                    // Ask for regular numbers
                    writeAndRead(client_sock, "Regular numbers?\n");
                    // Ask for good numbers
                    writeAndRead(client_sock, "Good numbers?\n");
                    // TODO Generate new number using threads
                    strcpy(server_message, "Is your number 1234?\n");
                }
            }
            puts("Client disconnected");
            fflush(stdout);

            close(client_sock);
            puts("Client socket closed");
            return 0;
        }
    }
    close(socket_desc);
    puts("Server socket closed");
    return 0;
}
