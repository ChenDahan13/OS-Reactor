#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include "st_reactor.h"

 
#define PORT "9034"   // Port we're listening on

// Get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Return a listening socket
int get_listener_socket(void)
{
    int listener;     // Listening socket descriptor
    int yes=1;        // For setsockopt() SO_REUSEADDR, below
    int rv;

    struct addrinfo hints, *ai, *p;

    // Get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        // Lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    freeaddrinfo(ai); // All done with this

    // If we got here, it means we didn't get bound
    if (p == NULL) {
        return -1;
    }

    // Listen
    if (listen(listener, 10) == -1) {
        return -1;
    }

    return listener;
}

// Chat between all clients
void clientHandler(Reactor* reactor, int client_fd) {
    
    char msg[BUFFER_SIZE] = {0};
    int numBytes = recv(client_fd, msg, BUFFER_SIZE, 0); // Receive messages
    if(numBytes <= 0) { // Something went wrong
        if(numBytes == 0) {
            printf("Server: fd %d disconnected\n", client_fd);
        } else {
            perror("recv");
        }
        close(client_fd);
        deleteFd(reactor, client_fd);
    } else {
        // Send to all clients
        printf("Server: fd %d got message: %s", client_fd, msg);
        for(int i = 0; i < reactor->event_count; i++) {
            if(reactor->fds[i].fd != client_fd) { // Send everyone, except from himself
                if(send(reactor->fds[i].fd, msg, numBytes, 0) == -1) {
                    perror("send");
                }
            }
        }
        printf("Message was sent to everyone\n");
    }
}

// Main
int main(void)
{
    int listener;     // Listening socket descriptor

    int newfd;        // Newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // Client address
    socklen_t addrlen;

    char buf[256];    // Buffer for client data

    char remoteIP[INET6_ADDRSTRLEN];


    // Set up and get a listening socket
    listener = get_listener_socket();

    if (listener == -1) {
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }

    struct pollfd fds[1];
    fds[0].fd = listener; // The socket of the server
    fds[0].events = POLLIN; // Alert for receiving data 

    Reactor *reactor = createReactor();
    startReactor(reactor);

    printf("Waiting for connections\n");

    // Main loop
    while(1) {
        
        int pollres = poll(fds, 1, -1);
        if(pollres == -1) {
            perror("Poll");
            return 1;
        }
        // Client trying to connect from the main socket
        if(fds[0].revents & POLLIN) {
            addrlen = sizeof(remoteaddr);
            newfd = accept(listener, (struct sockaddr*)&remoteaddr, &addrlen); // Accepts clients requests
            if(newfd == -1) {
                perror("Accept faild()");
                return 1;
            }
            printf("Client connected\n");
            addFd(reactor, newfd, clientHandler);
        }
    }
    return 0;
}



                