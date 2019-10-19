#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "service.h"

void generate_number(Guess * guesses, int lenght);

// SERVER

int start_server()
{
    int socket_desc;
    struct sockaddr_in server;

    // Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    // Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );
     
    // Bind
    if (bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("bind failed");
        return 1;
    }
    puts("bind done");
     
    // Listen
    listen(socket_desc , 3);
     
    puts("Waiting for incoming connections...");

    return socket_desc;
}

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

// THREADS
// TODO send lock by parameter??
// pthread_mutex_t lock;
// Is mutex worth it? Should use signals?
// All the threads could start on a random number and when a compatible number is found
// a signal can be emited that would kill/stop all the other threads
void * thread_rutine(void * args)
{
    // pthread_mutex_lock(&lock);
    int pos;
    Thread_data * data = args;
    Guess * guesses;
	printf("I'm the thread!\n");
    guesses = data -> guesses;
    pos = data -> pos;
    printf("I have: good: %d, regular: %d, new number: %s\n",guesses[pos].good, guesses[pos].regular, guesses[pos].number);
    generate_number(guesses, pos);
    // pthread_mutex_unlock(&lock);
	pthread_exit(NULL);
}

int run_threads(Thread_data data, int threads)
{
    int i;
    pthread_t tid[threads];

    // if (pthread_mutex_init(&lock, NULL) != 0) 
    // { 
    //     printf("\n mutex init has failed\n"); 
    //     return -1; 
    // }

    for (i = 0; i < threads; i++)
    {
        if(pthread_create(&tid[i], NULL, thread_rutine, &data) != 0)
        {
            return -1;
        }
        pthread_join(tid[i],NULL);
    }
    // pthread_mutex_destroy(&lock);
    return 0;
}

// NUMBER CALCULATION

void generate_number(Guess * guesses, int lenght)
{
    // TODO Implement all the logic that would calculate the new number
    int last_number, new_number;
    last_number = atoi(guesses[lenght].number);
    new_number = last_number + 1;

    // TODO check if it is really necessary that guess.number be an string
    sprintf(guesses[lenght+1].number, "%d", new_number);
}