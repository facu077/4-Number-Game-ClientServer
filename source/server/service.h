#ifndef _SERVICE_H_
#define _SERVICE_H_

// STRUCTS
typedef struct guess
{
    int regular;
    int good;
    char number[4];
} Guess;

typedef struct thread_data
{
    Guess * guesses;
    int pos;
} Thread_data;

// SERVER
int start_server();
char * writeAndRead(int socket, char * message);
int input_check(char * message, int number, int type);

// THREADS
int run_threads(Thread_data data, int threads);

// NUMBER LOGIC
int generate_number();

// CHILD
int run_child(int client_sock, char * ip, char *port, int pipe_fd[2], int threads_number);


#endif