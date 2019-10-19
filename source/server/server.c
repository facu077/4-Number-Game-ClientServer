#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "service.h"

int main(int argc , char *argv[])
{
    int socket_desc, client_sock, c, pos;
    struct sockaddr_in client;
    Guess * guesses;
    Thread_data data;
    char client_message[2000];
    char server_message[2000];
    int keep_playing = 1;
    char *ip;
    pid_t pid;

    socket_desc = start_server();
    c = sizeof(struct sockaddr_in);
    guesses = malloc(10 * sizeof *guesses);

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
            // TODO Generate random number
            strcpy(guesses[0].number, "1234");
            // TODO Improve this (and all) strcpy, strcat,strcat...
            strcpy(server_message, "Welcome ");
            strcat(server_message, client_message);
            strcat(server_message, ", you have played 8 times, with an average of 4 attempts per correct answer\n");
            strcat(server_message, "Is your number ");
            strcat(server_message, guesses[0].number);
            strcat(server_message, "?\n");
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
                    guesses[pos].regular = atoi(writeAndRead(client_sock, "Regular numbers?\n"));
                    // Ask for good numbers
                    guesses[pos].good = atoi(writeAndRead(client_sock, "Good numbers?\n"));
                    // TODO Generate new number using threads
                    data.guesses = guesses;
                    data.pos = pos;
                    //  Run the threads that will find the new number
                    if ((run_threads(data, 2) != 0))
                    {
                        printf("Error creating threads\n");
                        return -1;
                    }
                    pos++;
                    // Todo improve this message generation
                    strcpy(server_message, "Is your number ");
                    strcat(server_message, guesses[pos].number);
                    strcat(server_message, "?\n");
                }
            }
            puts("Client disconnected");
            fflush(stdout);

            close(client_sock);
            puts("Client socket closed");

            free(guesses);
            return 0;
        }
    }
    close(socket_desc);
    puts("Server socket closed");
    return 0;
}
