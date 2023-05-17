#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <poll.h>
#include "st_reactor.h"

bool isRunning = true;

Reactor* createReactor() {
    
    Reactor* r = (Reactor*)malloc(sizeof(Reactor));
    r->event_count = 1;
    r->size = 5;
    r->fds = (struct pollfd*)malloc((sizeof(struct pollfd))*(r->size));
    r->events = (Event*)malloc(sizeof(Event)*(r->size));
    return r;
}

void destroyReactor(Reactor* reactor) {
    if (reactor != NULL) {
        free(reactor->events);
        free(reactor->events);
        free(reactor);
    }
    else {
        printf("The function got NULL instead of reactor\n");
    }
}

/* The function adds the file discriptor and the function
   to the reactor */ 
void addFd(Reactor* reactor, int fd, handler_t handler) {
    
    if(reactor->event_count - 1 == reactor->size) { // In case the reactor is full
        reactor->size *= 2; // Double it
        reactor->events = realloc(reactor->events, sizeof(Event)*reactor->size); // More memory for events
        reactor->fds = realloc(reactor->fds, sizeof(struct pollfd)*reactor->size); // More memory for fds
    }
    // Adds the event
    Event event;
    event.fd = fd;
    event.handler = handler;
    reactor->events[reactor->event_count - 1] = event;
    
    // Adds the file descriptor
    reactor->fds[reactor->event_count - 1].fd = fd;
    reactor->fds[reactor->event_count - 1].events = POLLIN;
    
    reactor->event_count++;
    
}

// Finds the indes ot the file descriptor in the reactor
int findFd(Reactor *reactor, int fd) {
    for(int i = 0; i < reactor->event_count - 1; i++) {
        if(reactor->fds[i].fd == fd) // Found
            return i;
    }
    return -1;
}

void deleteFd(Reactor* reactor, int fd) {
    int index = findFd(reactor, fd);
    if(index != -1) { // Exist in the reactor
        
        // Copy the one from the end over this one
        reactor->fds[index] = reactor->fds[reactor->event_count - 1];
        reactor->events[index] = reactor->events[reactor->event_count - 1];
        reactor->event_count--;

    } else {
        printf("File descriptor does not exist in the reactor\n");
    }
    
}

/* This function will be executed by the thread */
void *reactorThread(Reactor* reactor) {
    // Runs the Reactor event loop
    while (isRunning) {
        
        int num_ready = poll(reactor->fds, reactor->event_count, -1);
        if (num_ready < 0) {
            perror("poll");
            exit(1);
        }
        
        // Checks which file descriptors have activity and calls their handlers
        for (int i = 0; i < reactor->event_count - 1; i++) {
            if (reactor->fds[i].revents & POLLIN)
                reactor->events[i].handler(reactor, reactor->events[i].fd);
        }
    }  
}

/* This function starts the reactor's thread */
void startReactor(Reactor* reactor) {
    int res = pthread_create(&(reactor->thread), NULL, reactorThread, reactor);
    if(res != 0) {
        perror("Create thread failed");
        return;
    }
}

/* Stops the action of the reactor */
void stopReactor(Reactor* reactor) {
    isRunning = false;
}

/* Waits untill reactor's thread will finish */
void waitFor(Reactor* reactor) {
    int res = pthread_join(reactor->thread, NULL);
    if(res != 0){
        perror("Join on reactor's thread failed");
        return;
    }
}
