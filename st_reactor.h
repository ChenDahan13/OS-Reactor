#ifndef ST_REACTOR_H
#define ST_REACTOR_H
#include <poll.h>
#include <pthread.h>

#define BUFFER_SIZE 256

typedef struct reactor Reactor;

typedef void (*handler_t)(Reactor*, int); // handler_t is a function pointer type

typedef struct event {
    int fd; // The file desriptor number
    handler_t handler; /* Function's pointer. 
                          The function will be executed when data received from fd */
} Event;

typedef struct reactor {
    Event *events; // Contains all fds with their corresponding functions
    struct pollfd *fds; 
    int event_count;
    int size;
    pthread_t thread;
} Reactor;


/*------- Functions -------*/
Reactor* createReactor();
void stopReactor(Reactor*);
void startReactor(Reactor*);
void addFd(Reactor*, int, handler_t);
void waitFor(Reactor*);
void destroyReactor(Reactor*);
void *reactorThread(Reactor*);
void deleteFd(Reactor*, int);
int findFd(Reactor*, int);

#endif