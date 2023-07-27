CC=gcc
CFLAGS=-g 
LDFLAGS=-g 

EXE=client server

all: $(EXE)

clean:
	rm -f $(EXE) *.o

client:client.o DieWithError.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ client.o DieWithError.o

server:server.o HandleTCPClient.o DieWithError.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ server.o HandleTCPClient.o DieWithError.o

