#include <gtk/gtk.h>
#include <webkit/webkit.h>
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

Address addr;
socklen_t len;
int cs;

// Data needed for the threads
Data data_thread;

// Mutex
pthread_mutex_t mutex;

// Name of the client
char *n;

// Flag to stop the threads
int loop;

// Indicating to the user in the text chat if we sending in p2p
char *indicator;

// List of connected users
GtkWidget *list;
char *users;

// Web wiew
GtkWidget *view;

// Number of connected users
int number_users;

// Label to print the number of users
GtkWidget *label_users;

// Pointer to the next char to read
int *next_char;

// Flag indicating we are in the conference
int conf;

// Flag indicating if we are in p2p or if we are broadcasting
int mode;

// name of the client to send the message to in p2p
char *peer;

/* Enum for the list of users */
enum
{
	LIST_ITEM = 0,
	N_COLUMNS
};

/* 
 * Displays a plain text message into the text view
 */
static void display_message(char *message, GtkWidget *view, char *tag) {
    GtkTextBuffer *buffer;
    GtkTextIter iter;

    // Gets the buffer and the iter from the view
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);

    // Insert the message at the beginning of the text field
   	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, message, -1, "lmarg", tag, NULL);
}

/* Called when the window is closed */
void destroy(GtkWidget *widget, gpointer data)
{
	gtk_main_quit ();
}

/*
 * Initializes the list of users
 * From: http://zetcode.com/tutorials/gtktutorial/gtktreeview/
 */
static void init_list(GtkWidget *list)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkListStore *store;

	/* Create a tree with one column and append it into a tree */
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("List Items", renderer, "text", LIST_ITEM, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

	/* Create a gtk list store model and set it to the tree */
	store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(store));

	/* The model will be destroyed automatically with the view */
	g_object_unref(store);
}

/*
 * Adds an item to the list
 * From: http://zetcode.com/tutorials/gtktutorial/gtktreeview/
 */
static void add_to_list(const gchar *str)
{
	GtkListStore *store;
	GtkTreeIter iter;

	/* Get the tree from the view */
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list)));

	/* Append the new user (str) to the tree */
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, LIST_ITEM, str, -1);
}

/*
 * Changes the broadcast mode to peer to peer with the selected user
 * From: http://zetcode.com/tutorials/gtktutorial/gtktreeview/
 */
void on_changed(GtkWidget *widget, gpointer label) 
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	char text[BUFSIZE];

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
		gtk_tree_model_get(model, &iter, LIST_ITEM, &peer,  -1);
		gtk_label_set_text(GTK_LABEL(label), peer);
		
		if(strcmp(peer, "Broadcast") == 0) {
			mode = BROADCAST;
			sprintf(indicator, "you");
			printf("%d Set broadcast mode, indicator = %s\n", mode, indicator);
			sprintf(text, "Broadcast mode activated\n");
		} else {
			mode = PTOP;
			sprintf(indicator, "you to %s", peer);
			printf("%d Set p2p mode with %s, indicator = %s\n", mode, peer, indicator);
			sprintf(text, "Peer_to_peer mode with %s activated\n", peer);
		}

		display_message(text, view, "red_fg");
	}
}

/*
 * Sends the message to the server
 */
void send_message(char *message) {
	printf("Sending: %s\n", message);
	if (sendto(cs, message, strlen(message), 0, (struct sockaddr *)&addr, len) < 0) {
		perror("ERROR - sendto failed");
		exit(-1);
	}
}

/*
 * Hooks the entry with the view
 * Used tutorial to create this from: https://developer.gnome.org/
 */
static void entry_activate_cb (GtkEntry *entry, GtkWidget *view)
{
	char tag[BUFSIZE];
    char text[BUFSIZE];
    char message[BUFSIZE];
    
    sprintf(text, "%s: %s\n", indicator, gtk_entry_get_text(entry));
    
    if(mode == BROADCAST) {
    	if(conf == IN_CONF) {
    		sprintf(message, "%d %s %s", mode, n, gtk_entry_get_text(entry));
    		sprintf(tag, "normal");
    		send_message(message);
    	} else {
    		sprintf(text, "You are outside the conference, you cannot broadcasts messages.\nPlease select a client before sending your message.\n");
    		sprintf(tag, "bold");
    	}
    } else {
    	sprintf(message, "%d %s %s %s", mode, n, peer, gtk_entry_get_text(entry));
    	sprintf(tag, "blue_fg");
    	send_message(message);
    }

	display_message(text, view, tag);
    gtk_entry_set_text (entry, "");
}

/*
 * Signal handler for the "active" signal of the Switch
 * Used tutorial to create this from: https://developer.gnome.org/
 */
static void activate_cb (GObject *switcher, GParamSpec *pspec, gpointer user_data)
{
	char buf[BUFSIZE];

	if (gtk_switch_get_active (GTK_SWITCH (switcher))) {
		sprintf(buf, "%d %s", JOIN, n);
		conf = IN_CONF;
		display_message("Joining conference\n", view, "red_fg");
	} else {
		sprintf(buf, "%d %s", LEAVE, n);
		conf = OUT_CONF;
		display_message("Leaving conference\n", view, "red_fg");
	}

	// Notifies the server
	send_message(buf);
}

/*
 * Creates the gui window
 * Used tutorials to create the window from: https://developer.gnome.org/
 * Used this tutorial to create the list: http://zetcode.com/tutorials/gtktutorial/gtktreeview/
 */
static GtkWidget* create_window(void)
{
	/* Declare variables */
	GtkWidget *window;
	GtkWidget *scroll;
	GtkWidget *entry;
	GtkWidget *label;
	GtkWidget *label_list;
	GtkWidget *switcher;
	GtkWidget *box;
	GtkWidget *box_message;
	GtkWidget *box_switch;
	GtkWidget *box_list;
	GtkWidget *box_window;
	GtkTreeSelection *selection;
	GtkTextIter iter;
	GtkTextBuffer *buffer;

	/* Create a window with a title, and a default size */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW (window), "Messenger");
	gtk_window_set_default_size(GTK_WINDOW (window), 660, 600);
	g_signal_connect(window, "delete-event", G_CALLBACK (gtk_main_quit), NULL);
	
	/* Create the boxes */
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
	box_message = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	box_switch = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	box_list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
	box_window = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	
	/* Create the list */
	list = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(list), FALSE);
	
	/*Create a label*/
	label = gtk_label_new("Conference Mode:");
	label_list = gtk_label_new("");
	label_users = gtk_label_new("");
	gtk_label_set_justify(GTK_LABEL(label_list), GTK_JUSTIFY_CENTER);
	
	/*Create a switch with a default active state*/
	switcher = gtk_switch_new();
	gtk_switch_set_active(GTK_SWITCH (switcher), TRUE);
	
	/* Create a new entry */
	entry = gtk_entry_new();

	/* Create a scrolled window */
	scroll = gtk_scrolled_window_new (NULL, NULL);
	g_object_set (scroll, "shadow-type", GTK_SHADOW_IN, NULL);

	/* Create the view to print the messages */
	view = gtk_text_view_new();
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	gtk_container_add(GTK_CONTAINER (scroll), view);
	gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);

	/* Create tags to apply polices to the texts */
	gtk_text_buffer_create_tag(buffer, "lmarg", "left_margin", 5, NULL);
	gtk_text_buffer_create_tag(buffer, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
	gtk_text_buffer_create_tag(buffer, "normal", "weight", PANGO_WEIGHT_NORMAL, NULL);
	gtk_text_buffer_create_tag(buffer, "blue_fg", "foreground", "blue", NULL);
	gtk_text_buffer_create_tag(buffer, "red_fg", "foreground", "red", NULL);

	/* Add a new message */
	gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, "You are connected in Broadcast mode\n", -1, "bold", "lmarg",  NULL);

	/* Hook the entry to the view */
	g_signal_connect (entry, "activate", G_CALLBACK (entry_activate_cb), view);
	
	/*Connecting the clicked signal to the callback function*/
	g_signal_connect (GTK_SWITCH (switcher), "notify::active", G_CALLBACK (activate_cb), window);

	/* Add the widgets to the boxes */
	gtk_box_pack_start(GTK_BOX (box_switch), label, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX (box_switch), switcher, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX (box), box_switch, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX (box), scroll, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX (box_message), entry, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX (box), box_message, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX (box_list), label_users, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX (box_list), list, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX (box_list), label_list, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX (box_window), box, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX (box_window), box_list, FALSE, FALSE, 5);
	
	/* Add the grid to the window */
	gtk_container_add(GTK_CONTAINER (window), box_window);
	
	/* Initialize the list */
	init_list(list);
	set_list_peers();
	
	/* Create the tree */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
	
	/* Hook the selection of en item of the list */
	g_signal_connect(selection, "changed", G_CALLBACK(on_changed), label_list);

	/* Show the window */
	gtk_widget_show_all(GTK_WIDGET (box_window));
	
	return window;
}

/*
 * Receives the message from the server
 */
void *receive_thread(void *data_thread) {
	int recvlen, action;
	
	// Pointer to functions
	funcptr *functions = malloc(sizeof(funcptr) * ACTIONS);
	functions[0] = &peer_to_peer;
	functions[1] = &broadcast;
	functions[2] = &add_user;
	functions[3] = &remove_user;

	// Buffer to receive messages
	char message[BUFSIZE];

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
			next_char[0] = 0;
			printf("Received: \"%s\"\n", message);
			if(strcmp(message, "You are disconnected") != 0) {
				// Parse the action wanted by the client
				action = parse_int(message, next_char);
				printf("Will perform action %d, next_char = %d\n", action, *next_char);
			
				// Check if the action is correct
				if((action >= ACTIONS) || (action < 0)) {
					printf("Client: unknown request\n");
				} else {
					// Process the request
					functions[action](message);
				}
			}
		}
	}
	
	free(functions);
	pthread_exit((void *) 0);
}

/*
 * Adds the connected users to the list
 */
void set_list_peers() {
	int i;
	char value[BUFSIZE];
	
	// Set all the values
	next_char[0] = 0;
	number_users = parse_int(users, next_char) + 1;
	
	// Set the label
	sprintf(value, "Connected users: %d", number_users);
	gtk_label_set_text(GTK_LABEL(label_users), value);
	
	// Set the list
	add_to_list("Broadcast");
	i = 1;
	while(i != number_users) {
		value[0] = '\0';
		parse_name(users, value, next_char);
		printf("names %s\n", value);
		add_to_list(value);
		++i;
	}
}

/*
 * Adds the user to the list of users
 */
void add_user(char *message) {
	char value[BUFSIZE];
	char text[BUFSIZE];

	parse_name(message, value, next_char);
	printf("New user: %s\n", value);

	// Print a message
	sprintf(text, "%s connected\n", value);
	display_message(text, view, "red_fg");
	
	// Add the new user
	add_to_list(value);
	
	// Increase number of users
	++number_users;
	
	// Set the label
	sprintf(value, "Connected users: %d", number_users);
	gtk_label_set_text(GTK_LABEL(label_users), value);
}

/*
 * Removes the user from the list
 * From: http://zetcode.com/tutorials/gtktutorial/gtktreeview/
 */
void remove_user(char *message) {
	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeIter iter;
	char *value;
	char name[BUFSIZE];
	char text[BUFSIZE];
	gboolean isvalid;

	// Get the user name
	parse_name(message, name, next_char);
	printf("Remove user: %s\n", name);

	// Get the list
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (list)));
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));

	// Get the first element
	isvalid = gtk_tree_model_get_iter_first(model, &iter);
		
	// Searching the user
	while(isvalid) {
		// Get the name of the user
		gtk_tree_model_get(model, &iter, LIST_ITEM, &value,  -1);
		
		if(strcmp(name, value) == 0) {
			// Remove the user
			gtk_list_store_remove(store, &iter);
			sprintf(text, "%s disconnected\n", name);
			display_message(text, view, "red_fg");
			return;
		}
		
		// Get the next user
		isvalid = gtk_tree_model_iter_next(model, &iter);
	}
}

/*
 * Receives a message in broadcast mode
 */
void broadcast(char *message) {
    char text[BUFSIZE];
   	char name[SIZENAME];
   	
   	parse_name(message, name, next_char);
   	sprintf(text, "%s: %s\n", name, consume(message, next_char));
   	
   	display_message(text, view, "normal");
}

/*
 * Receives a message in p2p mode
 */
void peer_to_peer(char *message) {
    char text[BUFSIZE];
   	char name[SIZENAME];
   	
   	parse_name(message, name, next_char);
   	parse_name(message, text, next_char);
   	sprintf(text, "%s to you: %s\n", name, consume(message, next_char));
   	
   	display_message(text, view, "blue_fg");
}

/*
 * Main function of the program
 */
int main(int argc, char **argv) {
	GtkWidget *window;
	pthread_attr_t attr;
	void *status;
	pthread_t receiver;
	int rc, recvlen;
	n = malloc(sizeof(char) * SIZENAME);
	char *buf = malloc(sizeof(char) * BUFSIZE);
	next_char = malloc(sizeof(int));
	
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
	
	// Set the flags
	loop = 0;
	conf = IN_CONF;
	mode = BROADCAST;
	
	// Set the indicator to its initial value
	indicator = malloc(sizeof(char*));
	sprintf(indicator, "you");
	
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
	
	// Get the list of connected users
	users = malloc(sizeof(char) * BUFSIZE);
	recvlen = recvfrom(cs, users, BUFSIZE, 0, (struct sockaddr *)&addr, &len);

	if (recvlen > 0) {
		// Add End caracter to the message
		users[recvlen] = '\0';
		printf("Received: \"%s\"\n", users);
	}

	// Set the threads
	rc = pthread_create(&receiver, &attr, receive_thread, (void *) &data_thread);
	if (rc){
		sprintf(n, "ERROR %d - Cannot create receiver thread\n", rc); 
		perror(n);
		free(n);
		return -1;
	}
	
	// Initialise GTK
	gtk_init(&argc, &argv);

	// Create the window
	window = create_window();
	
	gtk_widget_show(window);

	gtk_main();
	
	// User as closed the window we indicating we want to finish the execution
	loop = 1;
	
	// Notifies the server
	sprintf(buf, "%d %s", QUIT, n);
	printf("Sending: %s\n", buf);
	if (sendto(cs, buf, strlen(buf), 0, (struct sockaddr *)&addr, len) < 0) {
		perror("ERROR - sendto failed");
		return -1;
	}
	
	// Free attribute and wait for threads
	pthread_attr_destroy(&attr);
	
	rc = pthread_join(receiver, &status);
	if (rc) {
		sprintf(n, "ERROR %d - Cannot join receiver thread\n", rc); 
		perror(n);
		return -1;
	}
	
	pthread_mutex_destroy(&mutex);
	close(cs);
	free(buf);
	free(indicator);
	free(next_char);
	return 0;
}
