#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

char * writeAndRead(int socket, char * message);

int main(int argc , char *argv[])
{
    int socket_desc, client_sock, c;
    struct sockaddr_in server, client;
    char client_message[2000];
    char server_message[2000];
    int keep_playing = 1;
    char *ip;
    pid_t pid;
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );
     
    //Bind
    if (bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("bind failed");
        return 1;
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc , 3);
     
    puts("Waiting for incoming connections...");
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
            // Read client name
            read(client_sock, client_message, sizeof(client_message));
            // TODO Look in the log.txt file for the user data
            strcpy(server_message, "Welcome ");
            strcat(server_message, client_message);
            strcat(server_message, ", you have played 8 times, with an average of 4 attempts per correct answer\n");
            //Send the welcome message to client
            write(client_sock, server_message, strlen(server_message));
            while(keep_playing == 1)
            {
                // TODO Generate new number using threads
                // Ask the number to the client
                strcpy(client_message, writeAndRead(client_sock, "Is your number 1234?\n"));
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

// TODO move all functions to some service.c
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

/* const char * readAndWrite(int socket, char * message)
{
    int read_size;
    char answer[2000];
    // Write to client
    write(socket, message, strlen(message));
    // Read answer from client
    read_size = read(socket, answer, sizeof(answer));
    return answer;
} */