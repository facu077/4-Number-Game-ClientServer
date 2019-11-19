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
#include <sys/types.h>
#include <sys/wait.h>

#include "service.h"

void func(int signum) 
{ 
    wait(NULL); 
} 

int main(int argc , char *argv[])
{
    int socket_desc, client_sock, client_len, pipe_fd[2];
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
        printf("I FINISH\n");
        free(inter_message);
        puts("Logger process finished");
        return EXIT_SUCCESS;
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
            run_child(client_sock, ip, port, pipe_fd, number_of_threads);
            return EXIT_SUCCESS;
        }

        signal(SIGCHLD, func); 
    }
    close(socket_desc);
    kill(logger, SIGTERM);
    puts("Server socket closed");
    return 0;
}
