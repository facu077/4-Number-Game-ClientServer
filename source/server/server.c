#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "service.h"

int main(int argc , char *argv[])
{
    int socket_desc, client_sock, client_len, pos, pipe_fd[2];
    struct sockaddr_in client;
    char ip[INET6_ADDRSTRLEN];
    char * port;
    char * inter_message;
    pid_t pid, logger;

    socket_desc = start_server();
    client_len = sizeof(struct sockaddr_in);
    inter_message = calloc(200, sizeof(char));
    port = calloc(6, sizeof(char));
    // memset(inter_message, 0, sizeof inter_message);

    if (pipe(pipe_fd) < 0)
    {
		perror("pipe()");
		return -1;
	}

    logger = fork();
    if (logger == 0)
    {
        int fd, read_size, keep_listening = 1;
        puts("Logger process initialized");
        fd = open ("log.txt",O_RDWR|O_APPEND|O_CREAT, S_IRUSR|S_IWUSR);
        while (keep_listening == 1)
        {
            read_size = read(pipe_fd[0], inter_message, sizeof inter_message);
            if (strcmp(inter_message, "-1") == 0)
            {
                keep_listening = 0;
            }
            else
            {
                write(fd, inter_message, read_size);
            }
        }
        free(inter_message);
        puts("Logger process finished");
        return 0;
    }

    //Accept connection from an incoming client
    while ((client_sock = accept(socket_desc,(struct sockaddr *)&client,(socklen_t*)&client_len))>0)
    {
        puts("Connection accepted");

        if (getnameinfo((struct sockaddr*)&client,client_len,ip,sizeof(ip), 0,0,NI_NUMERICHOST) != 0)
        {
            perror("Cannot getaddrinfo");
            return -1;
        }
        // port = ntohs(client.sin_port);
        sprintf(port, "%d", ntohs(client.sin_port));
        printf("Client IP: %s and PORT: %s\n", ip, port);

        pid = fork();
        if (pid == 0)
        {
            Guess * guesses;
            Thread_data data;
            char client_message[2000], server_message[2000];
            int keep_playing = 1;

            guesses = malloc(10 * sizeof *guesses);

            // Ask client name
            strncpy(client_message, writeAndRead(client_sock, "Please enter your name: "), sizeof(client_message));
            // TODO Look in the log.txt file for the user data
            // Write user data to log.txt
            strncpy(inter_message, "Player: ", 9);
            strncat(inter_message, client_message, sizeof(client_message));
            strncat(inter_message, "; IP: ", 7);
            strncat(inter_message, ip, sizeof ip);
            strncat(inter_message, "; PORT: ", 9);
            strncat(inter_message, port, strlen(port));
            strncat(inter_message, ";\n", 3);
            write(pipe_fd[1], inter_message, strlen(inter_message));
            // TODO Generate random number
            strncpy(guesses[0].number, "1234", 5);
            // TODO Improve this (and all) strcpy, strcat,strcat...
            strncpy(server_message, "Welcome ", 9);
            strncat(server_message, client_message, sizeof(client_message));
            // strncat(server_message, ", you have played 8 times, with an average of 4 attempts per correct answer\n", 77);
            strncat(server_message, "\n", 2);
            strncat(server_message, "Is your number ", 16);
            strncat(server_message, guesses[0].number, 4);
            strncat(server_message, "?\n", 3);
            // Set welcome message to client with a random number
            while(keep_playing == 1)
            {
                // Ask the number to the client
                strncpy(client_message, writeAndRead(client_sock, server_message), sizeof(client_message));
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
                    strncpy(server_message, "Is your number ", 16);
                    strncat(server_message, guesses[pos].number, 4);
                    strncat(server_message, "?\n", 3);
                }
            }
            puts("Client disconnected");
            fflush(stdout);

            close(client_sock);
            puts("Client socket closed");
            free(inter_message);
            free(guesses);
            return 0;
        }
    }
    close(socket_desc);
    kill(logger, SIGTERM);
    puts("Server socket closed");
    return 0;
}
