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
    hints.ai_family = AF_INET6;    /* Allow IPv4 or IPv6 */
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

// CHILD

int run_child(int client_sock, char * ip, char *port, int pipe_fd[2], int threads_number)
{
    Guess * guesses;
    Thread_data data;
    char client_message[2000], server_message[2000];
    int keep_playing = 1;
    int valid_input = 0;
    int pos;
    time_t current_time;
    struct tm * time_info;
    char timeString[23];
    char * inter_message;
    inter_message = calloc(200, sizeof(char));


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
    strncat(inter_message, ip, strlen(ip));
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
            if ((run_threads(data, threads_number) != 0))
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