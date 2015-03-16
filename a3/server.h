#ifndef SERVER_H_
#define SERVER_H_

// Number of possible actions
#define ACTIONS 6

#include <netinet/in.h>

// Holds the users information
typedef struct User_t User;

// Address
typedef struct sockaddr_in Address;

struct User_t {
	// User's name
	char *name;
	
	// User's address
	Address addr;
	
	// Flag indicating he is in conference mode
	int conf;
};

// Function pointer
typedef void (*funcptr)(char *message, Address remaddr);

/*
 * Returns the id of the given username
 * Returns -1 if the user is not in the list
 */
int get_userid(char *name);

/*
 * Adds the user to the list of users
 */
void add_user(char *message, Address remaddr);

/*
 * Removes the users from the list
 */
void disconnect(char *message, Address remaddr);

/*
 * Sends the message to all the users
 */
void broadcast(char *message, Address remaddr);

/*
 * Sends the message to one user
 */
void peer_to_peer(char *message, Address remaddr);

/*
 * Holds the main loop of the program
 * Receives and processes the messages from the client
 */
void server();

/*
 * Free the global variables
 */
void free_vars();

#endif // SERVER_H_
