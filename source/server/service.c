#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <netdb.h>
#include "service.h"

int guess_number(Guess * guesses, int length);
void compare_numbers(char * old_number, Guess new_guess);
int is_valid(int number);

pthread_mutex_t lock;

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
    // 
    int pos, new_number;
    Thread_data * data = args;
    Guess * guesses;
	// printf("I'm the thread!\n");
    guesses = data -> guesses;
    pos = data -> pos;
    // printf("I have: good: %d, regular: %d, new number: %s\n",guesses[pos].good, guesses[pos].regular, guesses[pos].number);
    // generate_number(guesses, pos);
    new_number = guess_number(guesses, pos);
    pthread_mutex_lock(&lock);
    if (strcmp(guesses[pos+1].number, "") == 0)
    {
        // First thread!
        sprintf(guesses[pos+1].number, "%d", new_number);
        printf("%d has been inserted :)\n", new_number);
    }
    else
    {
        // Number already found by another thread
        printf("Someone already wrote there :(\n");
    }
    pthread_mutex_unlock(&lock);
	pthread_exit(NULL);
}

int run_threads(Thread_data data, int threads)
{
    int i;
    pthread_t tid[threads];

    if (pthread_mutex_init(&lock, NULL) != 0) 
    { 
        perror("mutex init has failed"); 
        return -1; 
    }

    for (i = 0; i < threads; i++)
    {
        if(pthread_create(&tid[i], NULL, thread_rutine, &data) != 0)
        {
            perror("thread_create has failed");
            return -1;
        }
        pthread_join(tid[i],NULL);
    }
    pthread_mutex_destroy(&lock);
    return 0;
}

// NUMBER CALCULATION

int generate_number()
{
    int number = 0000;
    // Random number between 1023 and 9876 without duplicates
    while(is_valid(number) == 0)
    {
        number = rand() % (9876 + 1 - 1023) + 1023;
    }
    // printf("number generated: %d\n", number);
    return number;
}

int is_valid(int number)
{
    char number_string[4];
    if (number == 0)
    {
        return 0;
    }
    sprintf(number_string, "%d", number);
    // printf("Im checking: %s\n", number_string);
    for (int i = 0; i < 3; i++)
    {
        for (int j = i + 1; j < 4; j++)
        {
            // printf("Checked %c and %c :)\n",number_string[i], number_string[j]);
            if (number_string[i] == number_string[j])
            {
                // Number with duplicate digits found
                // printf("YEP %c and %c are equals :)\n",number_string[i], number_string[j]);
                return 0;
            }
        }
    }
    return 1;
}

int guess_number(Guess * guesses, int length)
{
    int number_found = 0;
    // We start from a random valid number
    int new_number = generate_number();
    Guess new_guess;
    // If the user enters a wrong input the 'while' will be stuck forever
    // so we are going to use 'infinite_loop_control' to prevent it
    int infinite_loop_control = 0;
    while(number_found == 0)
    {
        infinite_loop_control += 1;
        // We need to check that the new number is valid
        int valid_number = 0;
        while(valid_number == 0)
        {
            if (new_number == 9876)
                new_number = 1023;
            else
                new_number += 1;
            valid_number = is_valid(new_number);
        }
        // printf("Puting %d in new_Guess\n", new_number);
        sprintf(new_guess.number, "%d", new_number);
        // For each result stored we check if the new number pass all the controls
        for(int pos = 0; pos <= length; pos++)
        {
            compare_numbers(guesses[pos].number, new_guess);
            printf("nr: %d = or: %d || ng: %d = og: %d\n",new_guess.regular, guesses[pos].regular, new_guess.good, guesses[pos].good);
            if (new_guess.regular != guesses[pos].regular || new_guess.good != guesses[pos].good)
            {
                number_found = 0;
                break;
            }
            else
            {
                number_found = 1;
            }
        }
    }
    printf("returning %s\n", new_guess.number);
    return atoi(new_guess.number);
}

void compare_numbers(char * old_number, Guess new_guess)
{
    new_guess.regular = 0;
    new_guess.good = 0;

    // We are going to modify the values so we need a backup
    char _old_number[4];
    strncpy(_old_number, old_number, 5);
    char _new_number[4];
    strncpy(_new_number, new_guess.number, 5);

    // First we go through the two numbers at the same time looking for "good"s
    // and remove the matches from the arrays to prevent duplicates
    printf("Comparing %s with %s\n",_old_number, _new_number);
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (_old_number[i] == _new_number[j] && _old_number[i] != '*' && _new_number[j] != '*')
            {
                new_guess.good += 1;
                _old_number[i] = '*';
                _new_number[j] = '*';
            }
        }
    }

    // Then we go through the rest of the list looking for regulars 
    // removing when found one to prevent duplicates
    for (int i = 0; i < 3; i++)
    {
        for (int j = i + 1; j < 4; j++)
        {
            if (_old_number[i] == _new_number[j] && _old_number[i] != '*' && _new_number[j] != '*')
            {
                new_guess.regular += 1;
                _old_number[i] = '*';
                _new_number[j] = '*';
            }
        }
    }
}

