#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <sys/wait.h>   /* for waitpid() */
#include <signal.h>     /* for sigaction() */

#define MAXPENDING 5    /* Maximum outstanding connection requests */

void DieWithError(char *errorMessage);  /* Error handling function */
void HandleTCPClient(int clntSocket);   /* TCP client handling function */
void ChildExitSignalHandler();     /* Function to clean up zombie child processes */
int CreateTCPServerSocket(unsigned short port); /* Create TCP server socket */
int AcceptTCPConnection(int servSock);  /* Accept TCP connection request */


// int main(int argc, char *argv[])
// {
//     int servSock;                    /* Socket descriptor for server */
//     int clntSock;                    /* Socket descriptor for client */
//     struct sockaddr_in echoServAddr; /* Local address */
//     struct sockaddr_in echoClntAddr; /* Client address */
//     unsigned short echoServPort;     /* Server port */
//     unsigned int clntLen;            /* Length of client address data structure */

//     if (argc != 2)     /* Test for correct number of arguments */
//     {
//         fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
//         exit(1);
//     }

//     echoServPort = atoi(argv[1]);  /* First arg:  local port */

//     /* Create socket for incoming connections */
//     if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
//         DieWithError("socket() failed");
      
//     /* Construct local address structure */
//     memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
//     echoServAddr.sin_family = AF_INET;                /* Internet address family */
//     echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
//     echoServAddr.sin_port = htons(echoServPort);      /* Local port */

//     /* Bind to the local address */
//     if (bind(servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
//         DieWithError("bind() failed");

//     /* Mark the socket so it will listen for incoming connections */
//     if (listen(servSock, MAXPENDING) < 0)
//         DieWithError("listen() failed");

//     for (;;) /* Run forever */
//     {
//         /* Set the size of the in-out parameter */
//         clntLen = sizeof(echoClntAddr);

//         /* Wait for a client to connect */
//         if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr, 
//                                &clntLen)) < 0)
//             DieWithError("accept() failed");

//         /* clntSock is connected to a client! */

//         printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

//         HandleTCPClient(clntSock);
//     }
//     /* NOT REACHED */
// }


/* Global so accessable by SIGCHLD signal handler */
unsigned int childProcCount = 0;   /* Number of child processes */

int main(int argc, char *argv[])
{
    int servSock;                    /* Socket descriptor for server */
    int clntSock;                    /* Socket descriptor for client */
    unsigned short echoServPort;     /* Server port */
    pid_t processID;                 /* Process ID from fork() */
    struct sigaction myAction;       /* Signal handler specification structure */
 
    if (argc != 2)     /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]);  /* First arg:  local port */

    servSock = CreateTCPServerSocket(echoServPort);

    /* Set ChildExitSignalHandler() as handler function */
    myAction.sa_handler =  ChildExitSignalHandler;
    if (sigfillset(&myAction.sa_mask) < 0)   /* mask all signals */
        DieWithError("sigfillset() failed");
    /* SA_RESTART causes interrupted system calls to be restarted */
    myAction.sa_flags = SA_RESTART;

    /* Set signal disposition for child-termination signals */
    if (sigaction(SIGCHLD, &myAction, 0) < 0)
        DieWithError("sigaction() failed");

    for (;;) /* run forever */
    {
	    clntSock = AcceptTCPConnection(servSock);

        /* Fork child process and report any errors */
        if ((processID = fork()) < 0)
            DieWithError("fork() failed");
        else if (processID == 0)  /* If this is the child process */
        {
            close(servSock);   /* Child closes parent socket file descriptor */
            HandleTCPClient(clntSock);
            exit(0);              /* Child process done */
        }

	printf("with child process: %d\n", (int) processID);
        close(clntSock);       /* Parent closes child socket descriptor */
        childProcCount++;      /* Increment number of outstanding child processes */
    }
    /* NOT REACHED */
}

void ChildExitSignalHandler()
{
    pid_t processID;           /* Process ID from fork() */

    while (childProcCount) /* Clean up all zombies */
    {
	processID = waitpid((pid_t) -1, NULL, WNOHANG);  /* Non-blocking wait */
	if (processID < 0)  /* waitpid() error? */
	    DieWithError("waitpid() failed");
	else if (processID == 0)  /* No child to wait on */
	    break;
	else
	    childProcCount--;  /* Cleaned up after a child */
    }
}