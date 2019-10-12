CC = gcc
CFLAGS = -g -Wall

TARGETS = client server

all: $(TARGETS)

client: source/client/client.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

server: source/server/server.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f $(TARGETS) *.o *~
