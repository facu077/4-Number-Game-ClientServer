#include<stdio.h>
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
 
int main(int argc , char *argv[])
{
    int socket_desc , client_sock , c , read_size;
    struct sockaddr_in server , client;
    char client_message[2000];
    char server_message[2000];
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
            // Receive messages from client
            // The first message should be the name of the user
            // so we send a welcome message with that
            read(client_sock, client_message, sizeof(client_message));
            // TODO Look in the log.txt file for the user data
            strcpy(server_message, "Welcome ");
            strcat(server_message, client_message);
            strcat(server_message, ", you have played 8 times, with an average of 4 attempts per correct answer");
            //Send the message back to client
            write(client_sock, server_message, strlen(server_message));
            while((read_size = recv(client_sock , client_message , sizeof(client_message) , 0)) > 0)
            {
                // TODO calculate new number
                // Send new number
                strcpy(server_message, "Is your number 1234?");
                write(client_sock, server_message, strlen(server_message));
                // Read user input
                read_size = read(client_sock, client_message, sizeof(client_message));
                if (strcmp(client_message, "yes") == 0)
                {
                    // GAME OVER
                    write(client_sock, "-1", 2);
                }
                else
                {
                    // Keep playing
                    // TODO Ask for regular numbers
                    // TODO Ask for good numbers
                    write(client_sock, "Keep playing", 12);
                }
            }
            
            if(read_size == 0)
            {
                puts("Client disconnected");
                fflush(stdout);
            }
            else if(read_size == -1)
            {
                perror("recv failed");
            }
            close(client_sock);
            puts("Client socket closed");
            return 0;
        }
    }
    close(socket_desc);
    puts("Server socket closed");
    return 0;
}