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
#include <time.h>

#include "service.h"

int main(int argc , char *argv[])
{
    int socket_desc, client_sock, client_len, pos, pipe_fd[2];
    struct sockaddr_in client;
    char ip[INET6_ADDRSTRLEN];
    char * port;
    char * inter_message;
    pid_t pid, logger;

    if (argc != 2)
    {
        printf("Usage: ./server <threads>\n");
        return 0;
    }

    int number_of_threads = atoi(argv[1]);
    socket_desc = start_server();
    client_len = sizeof(struct sockaddr_in);
    inter_message = calloc(200, sizeof(char));
    port = calloc(6, sizeof(char));

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
            read_size = read(pipe_fd[0], inter_message, 200);
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
        sprintf(port, "%d", ntohs(client.sin_port));
        printf("Client IP: %s and PORT: %s\n", ip, port);

        pid = fork();
        if (pid == 0)
        {
            Guess * guesses;
            Thread_data data;
            char client_message[2000], server_message[2000];
            int keep_playing = 1;
            int valid_input = 0;
            time_t current_time;
            struct tm * time_info;
            char timeString[23];

            guesses = calloc(10, sizeof *guesses);

            // Get current time
            time(&current_time);
            time_info = localtime(&current_time);
            strftime(timeString, sizeof(timeString), "%d/%m/%Y -- %H:%M:%S", time_info);

            // Ask client name
            strncpy(client_message, writeAndRead(client_sock, "Please enter your name: "), sizeof(client_message));
            // Write user data to log.txt
            strncpy(inter_message, "Player: ", 9);
            strncat(inter_message, client_message, sizeof(client_message));
            strncat(inter_message, "; IP: ", 7);
            strncat(inter_message, ip, sizeof ip);
            strncat(inter_message, "; PORT: ", 9);
            strncat(inter_message, port, strlen(port));
            strncat(inter_message, "; TIME: ", 9);
            strncat(inter_message, timeString, strlen(timeString));
            strncat(inter_message, "\n", 2);
            write(pipe_fd[1], inter_message, strlen(inter_message));
            // Generate random number
            sprintf(guesses[0].number, "%d", generate_number());
            // Set welcome message
            strncpy(server_message, "Welcome ", 9);
            strncat(server_message, client_message, sizeof(client_message));
            strncat(server_message, "\n", 2);
            strncat(server_message, "Is your number ", 16);
            strncat(server_message, guesses[0].number, 4);
            strncat(server_message, "?\n", 3);
            while(keep_playing == 1)
            {
                // Ask the number to the client
                while(valid_input != 1)
                {
                    strncpy(client_message, writeAndRead(client_sock, server_message), sizeof(client_message));
                    // Check for the input 0 - for numbers; 1 - for yes/no question
                    valid_input = input_check(client_message, 0, 1);
                    if (valid_input == 0)
                    {
                        strncpy(server_message, "Answer should be [y]es or [n]o\n", 32);
                    }
                }
                valid_input = 0;
                // Read user input
                if (strcmp(client_message, "yes") == 0 || strcmp(client_message, "y") == 0 ||
                    strcmp(client_message, "YES") == 0 || strcmp(client_message, "Y") == 0)
                {
                    // GAME OVER
                    write(client_sock, "-1", 3);
                    keep_playing = 0;
                }
                else
                {
                    // Keep playing
                    // Ask for regular numbers
                    strncpy(server_message, "Regular numbers?\n", 18);
                    while(valid_input != 1)
                    {
                        guesses[pos].regular = atoi(writeAndRead(client_sock, server_message));
                        // Check for the input 0 - for numbers; 1 - for yes/no question
                        valid_input = input_check("", guesses[pos].regular, 0);
                        if (valid_input == 0)
                        {
                            strncpy(server_message, "Regular numbers should be between 0 and 4\n", 43);
                        }
                    }
                    valid_input = 0;
                    // Ask for good numbers
                    strncpy(server_message, "Good numbers?\n", 15);
                    while(valid_input != 1)
                    {
                        guesses[pos].good = atoi(writeAndRead(client_sock, server_message));
                        // Check for the input 0 - for numbers; 1 - for yes/no question
                        valid_input = input_check("", guesses[pos].good, 0);
                        if (valid_input == 0)
                        {
                            strncpy(server_message, "Good numbers should be between 0 and 4\n", 40);
                        }
                    }
                    valid_input = 0;
                    data.guesses = guesses;
                    data.pos = pos;
                    //  Run the threads that will find the new number
                    if ((run_threads(data, number_of_threads) != 0))
                    {
                        printf("Error creating threads\n");
                        return -1;
                    }
                    pos++;
                    if (strcmp(guesses[pos].number, "-1") == 0)
                    {
                        strncpy(server_message, "It seems that you have entered an incorrect value.\nWould you like to start again?\n", 83);
                        while(valid_input != 1)
                        {
                            strncpy(client_message, writeAndRead(client_sock, server_message), sizeof(client_message));
                            // Check for the input 0 - for numbers; 1 - for yes/no question
                            valid_input = input_check(client_message, 0, 1);
                            if (valid_input == 0)
                            {
                                strncpy(server_message, "Answer should be [y]es or [n]o\n", 32);
                            }
                        }
                        valid_input = 0;
                        if (strcmp(client_message, "no") == 0 || strcmp(client_message, "NO") == 0 ||
                            strcmp(client_message, "n") == 0 || strcmp(client_message, "N") == 0)
                        {
                            // GAME OVER
                            write(client_sock, "-1", 3);
                            keep_playing = 0;
                        }
                        else
                        {
                            free(guesses);
                            guesses = calloc(10, sizeof *guesses);
                            pos = 0;
                            sprintf(guesses[0].number, "%d", generate_number());
                        }
                    }
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
