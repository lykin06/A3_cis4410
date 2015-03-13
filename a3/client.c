#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#include "values.h"
#include "client.h"
#include "parsing.h"

// Data needed for the threads
Data data_thread;

// Mutex
pthread_mutex_t mutex;

// Flag to stop the threads
int loop;

static void activate(GtkApplication* app, gpointer user_data) {
  GtkWidget *window;
  GtkWidget *label;

  window = gtk_application_window_new(app);
  label = gtk_label_new("Hello GNOME!");
  gtk_container_add(GTK_CONTAINER (window), label);
  gtk_window_set_title(GTK_WINDOW (window), "Welcome to GNOME");
  gtk_window_set_default_size(GTK_WINDOW (window), 200, 100);
  gtk_widget_show_all(window);
}

/*
 * Receives the message from the server
 */
void *receive_thread(void *data_thread) {
	int recvlen;

	// Buffer to receive messages
	char *message = malloc(sizeof(char) * BUFSIZE);

	// Set the data values
	Data *data = (Data *) data_thread;
	int cs = data->client_socket;
	Address addr = data->address;
	socklen_t len = data->addrlen;
	char *n = data->name;
	
	while(loop == 0) {
		// Empty buffer
		message[0] = '\0';
		recvlen = recvfrom(cs, message, BUFSIZE, 0, (struct sockaddr *)&addr, &len);

		if (recvlen > 0) {
			// Add End caracter to the message
			message[recvlen] = '\0';
			printf("Received: \"%s\"\n", message);
		}
	}
	
	free(message);
	
	pthread_exit((void *) 0);
}

/*
 * Sends the message to the server
 */
void *send_thread(void *data_thread) {
	// Buffer to send messages
	char *buf = malloc(sizeof(char) * BUFSIZE);
	char *message = malloc(sizeof(char) * MESSAGE);

	// Set the data values
	Data *data = (Data *) data_thread;
	int cs = data->client_socket;
	Address addr = data->address;
	socklen_t len = data->addrlen;
	char *n = data->name;
	
	while(loop == 0) {
		printf("Enter your message: ");
		scanf("%s", message);

		if (strlen(message) > 0) {
			if(strcmp(message, "quit") == 0) {
				loop = 1;
				sprintf(buf, "%d %s", QUIT, n);
			} else {
				sprintf(buf, "%d %s %s", BROADCAST, n, message);
			}
			
			printf("Sending: \"%s\"\n", buf);
			if (sendto(cs, buf, strlen(buf), 0, (struct sockaddr *)&addr, len) < 0) {
				perror("ERROR - sendto failed");
				free(message);
				free(buf);
				pthread_exit((void *) -1);
			}
		}
	}
	
	free(message);
	free(buf);
	pthread_exit((void *) 0);
}

/*
 * Main function of the program
 */
int mainold(int argc, char **argv) {
	pthread_attr_t attr;
	void *status;
	pthread_t receiver;
	pthread_t sender;
	int cs, rc;
	Address addr;
	socklen_t len;
	char *n = malloc(sizeof(char) * SIZENAME);
	char *buf = malloc(sizeof(char) * BUFSIZE);
	
	// Set client's name
	if(argc > 0) {
		n = argv[1];
	} else {
		printf("Enter your name: ");
		scanf("%s", n);
	}
	
	while(strlen(n) > SIZENAME - 1) {
		printf("WARNING - Your name must be under %d characters\nPlease, enter a new one: ", (SIZENAME - 1));
		scanf("%s", n);
	}
	
	// Set the loop flag
	loop = 0;
	
	// Set lenght of an address
    len = sizeof(Address);
	
	// Set the address to the server
	memset((char *)&addr, 0, len);
	addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);
    
    // create a UDP socket
	if ((cs = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return -1;
	}
    
	// Initialize Mutex
	pthread_mutex_init(&mutex, NULL);
	
	// Initialize and set thread detached attribute
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	
	// Set the data
	data_thread.client_socket = cs;
	data_thread.address = addr;
	data_thread.addrlen = len;
	data_thread.name = n;
	
	// Sends our name to the server
	sprintf(buf, "%d %s", CONNECT, n);
	
	printf("Sending: %s\n", buf);
	if (sendto(cs, buf, strlen(buf), 0, (struct sockaddr *)&addr, len) < 0) {
		perror("ERROR - sendto failed");
		free(n);
		free(buf);
		return -1;
	}

	// Set the threads
	rc = pthread_create(&receiver, &attr, receive_thread, (void *) &data_thread);
	if (rc){
		sprintf(n, "ERROR %d - Cannot create receiver thread\n", rc); 
		perror(n);
		free(n);
		return -1;
	}
	
	rc = pthread_create(&sender, &attr, send_thread, (void *) &data_thread);
	if (rc){
		sprintf(n, "ERROR %d - Cannot create sender thread\n", rc); 
		perror(n);
		close(cs);
		free(n);
		return -1;
	}
	
	// Free attribute and wait for threads
	pthread_attr_destroy(&attr);
	
	rc = pthread_join(receiver, &status);
	if (rc) {
		sprintf(n, "ERROR %d - Cannot join receiver thread\n", rc); 
		perror(n);
		close(cs);
		free(n);
		return -1;
	}
	
	rc = pthread_join(sender, &status);
	if (rc) {
		sprintf(n, "ERROR %d - Cannot join sender thread\n", rc); 
		perror(n);
		close(cs);
		free(n);
		return -1;
	}
	
	pthread_mutex_destroy(&mutex);
	close(cs);
	free(n);
	free(buf);
	return 0;
}

int main (int argc, char **argv){
  GtkApplication *app;
  int status;

  app = gtk_application_new(NULL, G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run(G_APPLICATION (app), argc, argv);
  g_object_unref(app);

  return status;
}
