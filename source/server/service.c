#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <netdb.h>
#include "service.h"

void generate_number(Guess * guesses, int lenght);

// SERVER

int start_server()
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Stream socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */


    s = getaddrinfo(NULL, "8888", &hints, &result);
    if (s != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    /* getaddrinfo() returns a list of address structures.
        Try each address until we successfully bind(2).
        If socket(2) (or bind(2)) fails, we (close the socket
        and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
        {
            continue;
        }
        if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            /* Success */
            puts("Bind done");
            break;
        }

        close(sfd);
    }

    if (rp == NULL)
    {
        /* No address succeeded */
        perror("Could not bind");
        exit(EXIT_FAILURE);
    }

    if (listen(sfd, 3) < 0)
    {
        perror("Error in syscall listen");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(result);
    puts("Waiting for incoming connections...");
    return sfd;
}

char * writeAndRead(int socket, char * message)
{
    int read_size;
    char * answer;
    answer = calloc(2000, sizeof(char));
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