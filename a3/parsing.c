#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "parsing.h"
#include "values.h"

/*
 * Parses the first integer of the message
 */
int parse_int(char *message, int *next_char) {
	// char at the next_char position
	char c;
	
	// Buffer to store the chars we read
	char *buf;
	
	// Some integers
	int max, i, result, nc;
	
	// Sat the values
	buf = malloc(sizeof(char)*BUFSIZE);
	max = strlen(message);
	nc = next_char[0];
	
	// Read the message until reaching a space
	for(i = 0; nc < max; ++i, ++nc) {
		c = message[nc];
		if(isspace(c)) {
			buf[i] = '\0';
			result = atoi(buf);
			free(buf);
			
			// increase the next_char value to not read the space
			next_char[0] = nc + 1;
			
			return result;
		} else {
			buf[i] = c;
		}
	}
	
	// Reach end of the message
	result = atoi(buf);
	free(buf);
	return result;
}

/*
 * Parses the user name from the message
 */
void parse_name(char *message, char *name, int *next_char) {
	// char at the next_char position
	char c;
	
	// Some integers
	int max, i, nc;
	
	// Set the values
	max = strlen(message);
	nc = next_char[0];
	
	// Read the message until reaching a space
	for(i = 0; nc < max; ++i, ++nc) {
		c = message[nc];
		if(isspace(c)) {
			name[i] = '\0';
			
			// increase the next_char value to not read the space
			next_char[0] = nc + 1;
			
			// Leaves the function
			return;
		} else {
			name[i] = c;
		}
	}
	
	// Reach end of the message
	name[i] = '\0';
}

/*
 * Puts the rest of the message into a buffer
 */
char *consume(char *message, int *next_char) {
	// Buffer to store the chars we read
	char *buf;
	
	// Some integers
	int max, i, nc;
	
	// Sat the values
	buf = malloc(sizeof(char)*BUFSIZE);
	max = strlen(message);
	nc = next_char[0];
	
	// Read the message until reaching the end of the end of the message
	for(i = 0; nc < max; ++i, ++nc) {
		buf[i] = message[nc];
	}
	
	buf[i] = '\0';
	return buf;
}

