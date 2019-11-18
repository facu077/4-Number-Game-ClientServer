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
void compare_numbers(char * old_number, Guess * new_guess);
int is_valid(int number);
void clean_string(char *s);

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
    read_size = read(socket, answer, 2000);

    if(read_size == -1)
    {
        perror("read failed");
    }
    clean_string(answer);
    return answer;
}

// For telnet support
void clean_string(char *s)
{
    char *p2 = s;
    while(*s != '\0') {
        if(*s != '\r' && *s != '\n') {
            *p2++ = *s++;
        } else {
            ++s;
        }
    }
    *p2 = '\0';
}


// Check for the input type: 0 - for numbers; type: 1 - for yes/no question
// return 1 for invalid - 0 for valid
int input_check(char * message, int number, int type)
{
    switch (type)
    {
        case 0:
            if (number >= 0 && number <= 4)
            {
                return 1;
            }
            break;
        ;
        case 1:
            if (strcmp(message, "yes") == 0 || strcmp(message, "no") == 0
                || strcmp(message, "y") == 0 || strcmp(message, "n") == 0 
                || strcmp(message, "YES") == 0 || strcmp(message, "NO") == 0
                || strcmp(message, "Y") == 0 || strcmp(message, "N") == 0)
            {
                return 1;
            }
            break;
        ;
        default:
            puts("Error: Invalid input type\n");
            return -1;
        ;
    }
    return 0;
}

// THREADS
void * thread_rutine(void * args)
{
    // 
    int pos, new_number;
    Thread_data * data = args;
    Guess * guesses;

    guesses = data -> guesses;
    pos = data -> pos;

    new_number = guess_number(guesses, pos);
    pthread_mutex_lock(&lock);
    if (strcmp(guesses[pos+1].number, "") == 0)
    {
        // First thread!
        sprintf(guesses[pos+1].number, "%d", new_number);
        printf("%d has been inserted\n", new_number);
    }
    else
    {
        // Number already found by another thread
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
    srand ( time(NULL) );
    // Random number between 1023 and 9876 without duplicates
    while(is_valid(number) == 0)
    {
        number = rand() % (9876 + 1 - 1023) + 1023;
    }
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
    for (int i = 0; i < 3; i++)
    {
        for (int j = i + 1; j < 4; j++)
        {
            if (number_string[i] == number_string[j])
            {
                // Number with duplicate digits found
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
        sprintf(new_guess.number, "%d", new_number);
        // For each result stored we check if the new number pass all the controls
        for(int pos = 0; pos <= length; pos++)
        {
            compare_numbers(guesses[pos].number, &new_guess);
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
        if (infinite_loop_control > 9876)
        {
            // After checking all the posibilities no number found
            return -1;
        }
    }
    return atoi(new_guess.number);
}

void compare_numbers(char * old_number, Guess * new_guess)
{
    int regular = 0;
    int good = 0;

    // We are going to modify the values so we need a backup
    char * _old_number;
    char * _new_number;
    _old_number = calloc(4, sizeof(char));
    _new_number = calloc(4, sizeof(char));
    strncpy(_old_number, old_number, 4);
    strncpy(_new_number, new_guess->number, 4);

    // First we go through the two numbers at the same time looking for "good"s
    // and remove the matches from the arrays to prevent duplicates
    for (int i = 0; i < 4; i++)
    {
        if (_old_number[i] == _new_number[i] && _old_number[i] != '*' && _new_number[i] != '*')
        {
            good += 1;
            _old_number[i] = '*';
            _new_number[i] = '*';
        }
    }

    // Then we go through the rest of the list looking for "regular"s 
    // removing when found one to prevent duplicates

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (_old_number[i] == _new_number[j] && _old_number[i] != '*' && _new_number[j] != '*' && i != j)
            {
                regular += 1;
                _old_number[i] = '*';
                _new_number[j] = '*';
            }
        }
    }
    new_guess->regular = regular;
    new_guess->good = good;
}

