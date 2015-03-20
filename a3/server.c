#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include "values.h"
#include "server.h"
#include "parsing.h"

// Address of the server
Address myaddr;

// Socket of the server
int server_socket;

// Lenght of an address
socklen_t addrlen;

// Integer holding the position of the next char to read
int *next_char;

// List of connected users
User *users;

// Number of connected users
int number_users;
int alloc;

// user's action
int action;

/*
 * Returns the id of the given username
 * Returns -1 if the user is not in the list
 */
int get_userid(char *name) {
	printf("get_userid, name %s\n", name);
	int i;
	
	for(i = 0; i < number_users; ++i) {
		printf("loop %d, test %s\n", i, users[i].name);
		if(strcmp(users[i].name, name) == 0) {
			return i;
		}
	}
	
	// Not in the list
	return -1;
}

/*
 * Adds the user to the list of users
 */
void add_user(char *message, Address remaddr) {
	int i;
	char *names;
	char *buf = malloc(sizeof(char) * SIZENAME);
	
	// Sends the list of user 
	if(number_users > 0) {
		// Sends the list of users to the new client
		names = malloc(sizeof(char) * BUFSIZE);
		names[0] = '\0';
		sprintf(names, "%d ", number_users);
		for(i = 0; i < number_users; ++i) {
			sprintf(buf, "%s ", users[i].name);
			strcat(names, buf);
		}
		printf("Sending: \"%s\"\n", names);
		if (sendto(server_socket, names, strlen(names), 0, (struct sockaddr *)&remaddr, addrlen) < 0) {
			perror("ERROR - sendto failed");
			exit(-1);
		}
		free(names);
		
		// Notifies the others
		for(i = 0; i < number_users; ++i) {
			printf("Sending: \"%s\" to %s\n", message, users[i].name);
	
			if (sendto(server_socket, message, strlen(message), 0, (struct sockaddr *)&(users[i].addr), addrlen) < 0) {
				perror("ERROR - sendto failed");
				exit(-1);
			}
		}
	} else {
		sprintf(buf, "%d ", 0);
		printf("Sending: \"%s\"\n", buf);
		if (sendto(server_socket, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, addrlen) < 0) {
			perror("ERROR - sendto failed");
			exit(-1);
		}
	}
	
	// Gets the name of the user
	buf = consume(message, next_char);
	i = strlen(buf);
	printf("Adding user %s size %d\n", buf, i);
	
	// Adds the user to the list
	if((number_users % MAXUSERS) >= MAXUSERS) {
		// Adding new slots
		++alloc;
		users = realloc(users, sizeof(User) * MAXUSERS * alloc);
	}
	users[number_users].name = buf;
	users[number_users].addr = remaddr;
	users[number_users].conf = IN_CONF;
	printf("Added %s\n", users[number_users].name);
	
	// Increase number of users
	++number_users;
}

/*
 * Removes the users from the list
 */
void disconnect(char *message, Address remaddr) {
	int i;
	
	// Flag indicating we removes the user
	int removed = 0;

	// Sends the message to all the others
	broadcast(message, remaddr);
	
	// Gets the name of the user
	char *buf = consume(message, next_char);
	
	// Notifies the user
	sprintf(message, "You are disconnected");
	if (sendto(server_socket, message, strlen(message), 0, (struct sockaddr *)&remaddr, addrlen) < 0) {
		perror("ERROR - sendto failed");
		exit(-1);
	}
	
	// Removes the user from the list
	if(number_users > 1) {
		for(i = 0; i < number_users; ++i) {
			if(removed > 0) {
				// Shifts the user
				users[i - 1] = users[i];
			} else {
				// We doing nothing until we found the user
				if(strcmp(buf, users[i].name) == 0) {
					++removed;
				}
			}
		}
	}
	
	--number_users;
	printf("User removed, %d users left\n", number_users);
}

/*
 * Sends the message to all the users
 */
void broadcast(char *message, Address remaddr) {
	printf("Broadcast\n");
	int i;
	int id;
	char *n = malloc(sizeof(char) * BUFSIZE);
	
	// Get user name
	parse_name(message, n, next_char);
	id = get_userid(n);
	printf("Sender: %s, id: %d, next_char = %d\n", n, id, *next_char);
	
	if(id >= 0) {
		printf("Known user\n");
		if(number_users > 1) {
			printf("Enough users\n");
			for(i = 0; i < number_users; ++i) {
				printf("loop %d, test %d\n", i, id);
				if(i != id) {
					if((users[i].conf == IN_CONF) || (action != BROADCAST)) {
						printf("Sending: \"%s\" to %s\n", message, users[i].name);
			
						if (sendto(server_socket, message, strlen(message), 0, (struct sockaddr *)&(users[i].addr), addrlen) < 0) {
							perror("ERROR - sendto failed");
							exit(-1);
						}
					} else {
						printf("%s out of conference\n", users[i].name);
					}
				} else {
					printf("Sender %s\n", users[id].name);
				}
			}
		} else {
			printf("Not enough users\n");
		}
	} else {
		printf("Unknown user\n"); 
	}
	
	free(n);
}

/*
 * Sends the message to one user
 */
void peer_to_peer(char *message, Address remaddr) {
	int id_dest, id;
	//char *buf; 
	char *dest;
	char *name = malloc(sizeof(char) * BUFSIZE);
	
	// Get the sender
	parse_name(message, name, next_char);
	id = get_userid(name);
	
	if(id < 0) {
		printf("Unknown user\n");
		// Leaves the function
		free(name);
		return;
	}
	
	// Get the destination
	dest = malloc(sizeof(char) * BUFSIZE);
	parse_name(message, dest, next_char);
	id_dest = get_userid(dest);
	
	if(id_dest < 0) {
		printf("Unknown dest\n");
		// Leaves the function
		free(name);
		free(dest);
		return;
	}
		
	// Sends message to dest
	if (sendto(server_socket, message, strlen(message), 0, (struct sockaddr *)&(users[id_dest].addr), addrlen) < 0) {
		perror("ERROR - sendto failed");
		exit(-1);
	}
	
	free(dest);
	free(name);
}

void join_conf(char *message, Address remaddr) {
	// Gets the name of the user
	char *buf = consume(message, next_char);
	
	// Gets the id of the user
	int id = get_userid(buf);
	
	// Add the user to the conference
	users[id].conf = IN_CONF;
	
	printf("%d Added user %s to the conference\n", users[id].conf, buf);
}

void leave_conf(char *message, Address remaddr) {
	// Gets the name of the user
	char *buf = consume(message, next_char);
	
	// Gets the id of the user
	int id = get_userid(buf);
	
	// Add the user to the conference
	users[id].conf = OUT_CONF;
	
	printf("%d Removed user %s out of the conference\n", users[id].conf, buf);
}

/*
 * Free the global variables
 */
void free_vars() {
	free(users);
	free(next_char);
	close(server_socket);
}

/*
 * Holds the main loop of the program
 * Receives and processes the messages from the client
 */
void server() {
	int recvlen;
	char *message;
	Address remaddr;
	
	// Set local the variables
	message = malloc(sizeof(char) * BUFSIZE);
	
	// Pointer to functions
	funcptr *functions = malloc(sizeof(funcptr) * ACTIONS);
	functions[0] = &peer_to_peer;
	functions[1] = &broadcast;
	functions[2] = &add_user;
	functions[3] = &disconnect;
	functions[4] = &join_conf;
	functions[5] = &leave_conf;

	while(1) {
		printf("Server: Waiting on port %d\n", PORT);
		recvlen = recvfrom(server_socket, message, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
		
		// Check if the message is not empty before processing it
		if (recvlen > 0) {
			// Add End caracter to the message
			message[recvlen] = '\0';
			next_char[0] = 0;
			printf("Server: Received message: \"%s\"\n", message);
			
			// Parse the action wanted by the client
			action = parse_int(message, next_char);
			printf("Will perform action %d, next_char = %d\n", action, *next_char);
			
			// Check if the action is correct
			if((action >= ACTIONS) || (action < 0)) {
				printf("Server: unknown request\n");
			} else {
				// Process the request
				functions[action](message, remaddr);
			}			
		} else {
			printf("Server: Cannot process request, Not enough data received.\n");
		}
	}
	
	free(functions);
}

/*
 * Main function
 * Set all the needed values and start the main loop
 */
int main(int argc, char **argv) {
	// Set the global variables
	addrlen = sizeof(struct sockaddr_in);
	users = malloc(sizeof(User) * MAXUSERS);
	next_char = malloc(sizeof(int));
	number_users = 0;
	alloc = 1;
	
	// Set server address and port
	memset((char *)&myaddr, 0, addrlen);
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(PORT);
	
	// create a UDP socket
	if ((server_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("ERROR - cannot create socket\n");
		free_vars();
		return -1;
	}

	// Bind the socket to the address
	if (bind(server_socket, (struct sockaddr *)&myaddr, addrlen) < 0) {
		perror("ERROR - bind failed\n");
		free_vars();
		close(server_socket);
		return -1;
	}
	
	server();

	return 0;
}
