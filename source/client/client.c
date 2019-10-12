#include<stdio.h> //printf
#include<string.h> //strlen
#include<sys/socket.h> //socket
#include<arpa/inet.h> //inet_addr
#include <unistd.h> // for close
 
int main(int argc , char *argv[])
{
    int sock, read_size;
    struct sockaddr_in server;
    char message[1000] , server_reply[2000];
     
    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons( 8888 );
 
    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }
     
    puts("Connected\n");
    memset(message, 0, sizeof message);

    // Input player name
    puts("Please enter your name:");

    // write to the server
    scanf("%s", message);
    write(sock, message, strlen(message));

    //keep communicating with server
    while(strcmp(server_reply, "-1") != 0)
    {
        // read from the server
        read_size = read(sock, server_reply, sizeof server_reply);
        if (read_size < 0)
        {
            puts("recv failed");
        }
        puts("/n");
        write(STDOUT_FILENO, server_reply, read_size);
        // write to the server
        scanf("%s", message);
        write(sock, message, strlen(message));
    }
    
    close(sock);
    return 0;
}