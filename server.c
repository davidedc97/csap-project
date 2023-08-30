#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <sys/wait.h>   /* for waitpid() */
#include <signal.h>     /* for sigaction() */

#define MAXPENDING 5    /* Maximum outstanding connection requests */
#define MAXPATHSIZE 512 /* Maximum size of root path */
#define MAXCMDS 64      /* Maximum number of additional commands */

void DieWithError(char *errorMessage);  /* Error handling function */
void HandleTCPClient(int clntSocket);   /* TCP client handling function */
void ChildExitSignalHandler();     /* Function to clean up zombie child processes */
int CreateTCPServerSocket(unsigned short port); /* Create TCP server socket */
int AcceptTCPConnection(int servSock);  /* Accept TCP connection request */

/* Define a struct to contain configuration variables */
typedef struct config {
  int port;
  char rootPath[MAXPATHSIZE];
  char* commands[MAXCMDS];
} CONFIG;

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
        fprintf(stderr, "Usage:  %s <CONFIG_FILE>\n", argv[0]);
        exit(1);
    }

    /* Read and parse config file */
    FILE* f = fopen(argv[1], "r");

    if(f == NULL){
        fprintf(stderr, "Cannot open CONFIG_FILE provided\n");
        exit(1);
    }

    char line[MAXPATHSIZE];
    size_t len = 0;
    int bytes = 0;
    CONFIG* conf = (CONFIG*)malloc(sizeof(CONFIG));

    printf("arrivo qui\n");
    /* PORT */
    fgets(line, MAXPATHSIZE, f);
    printf("Line: %s\n", line);
    fgets(line, MAXPATHSIZE, f);
    printf("Line: %s\n", line);
    printf("Cast: %d\n", atoi(line));
    conf->port = atoi(line);
    printf("conf->port: %d\n", conf->port);
    /* ROOT */
    fgets(line, MAXPATHSIZE, f);
    printf("Line: %s\n", line);
    fgets(line, MAXPATHSIZE, f);
    printf("Line: %s\n", line);
    strcpy(conf->rootPath, line);
    printf("conf->rootPath: %s\n", conf->rootPath);
    /* COMMANDS */
    fgets(line, MAXPATHSIZE, f);
    // while ((read = getline(&line, &len, fp)) != -1) {
    //     printf("Retrieved line of length %zu:\n", read);
    //     printf("%s", line);
    // }


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