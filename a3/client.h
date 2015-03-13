#ifndef CLIENT_H_
#define CLIENT_H_

#include <netinet/in.h>

// Address
typedef struct sockaddr_in Address;

// Struct to store all the data threads
typedef struct thread_data_t Data;

struct thread_data_t {
	// Socket
	int client_socket;

	// Address
	Address address;

	// Lenght of an address
	socklen_t addrlen;

	// Name of the client;
	char *name;
};

#endif // CLIENT_H_

