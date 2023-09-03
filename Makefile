CC=gcc
CFLAGS=-g 
LDFLAGS=-g 

EXE=client server cleanObjs

all: $(EXE)

clean:
	rm -f $(EXE) *.o

cleanObjs:
	rm -f *.o

client:client.o DieWithError.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ client.o DieWithError.o

server:server.o HandleTCPClient.o DieWithError.o CreateTCPServerSocket.o AcceptTCPConnection.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ server.o HandleTCPClient.o DieWithError.o CreateTCPServerSocket.o AcceptTCPConnection.o 

