TARGET = clients server
LIBS = -lm
CC = gcc
CFLAGS = -g -Wall -pthread
GTK = `pkg-config --cflags --libs gtk+-3.0 webkitgtk-3.0`

.PHONY: clean default all

default: $(TARGET)
all: default

OBJECTS_CLIENTS = client.c parsing.c
OBJECTS_SERVER = server.c parsing.c
HEADERS = server.h client.h parsing.h values.h
OBJECTS = $(OBJECTS_SERVER) $(OBJECTS_CLIENTS)



.PRECIOUS: $(TARGET) $(OBJECTS)

clients: $(OBJECTS_CLIENTS)
	$(CC) $(OBJECTS_CLIENTS) $(GTK) $(CFLAGS) $(LIBS) -o $@

server: $(OBJECTS_SERVER)
	$(CC) $(OBJECTS_SERVER) $(CFLAGS) $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
	-rm -f *~
