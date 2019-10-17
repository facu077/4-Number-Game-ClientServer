CC = gcc
CFLAGS = -g -Wall -pthread
TARGETS = client server

all: $(TARGETS)

client: source/client/client.c source/client/service.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

server: source/server/server.c source/server/service.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f $(TARGETS) *.o *~
