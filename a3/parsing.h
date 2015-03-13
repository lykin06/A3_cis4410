/*
 * Parses the first integer of the message
 */
int parse_int(char *message, int *next_char);

/*
 * Returns the user name
 */
void parse_name(char *message, char *name, int *next_char);

/*
 * Returns the rest of the message
 */
char *consume(char *message, int *next_char);

