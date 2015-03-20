#ifndef CLIENT_H_
#define CLIENT_H_

#include <netinet/in.h>

// Number of possible actions
#define ACTIONS 4

// Address
typedef struct sockaddr_in Address;

// Struct to store all the data threads
typedef struct thread_data_t Data;

// Function pointer
typedef void (*funcptr)(char *message);

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

/*
 * Adds the connected users to the list
 */
void set_list_peers();

/*
 * Adds the user to the list of users
 */
void add_user(char *message);

/*
 * Removes the user from the list
 */
void remove_user(char *message);

/*
 * Receives a message in broadcast mode
 */
void broadcast(char *message);

/*
 * Receives a message in p2p mode
 */
void peer_to_peer(char *message);

/*
 * Sends the message to the server
 */
void send_message(char *message);

#endif // CLIENT_H_

