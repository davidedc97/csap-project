#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <sys/wait.h>   /* for waitpid() */
#include <signal.h>     /* for sigaction() */
#include <dirent.h>     /* for directory checks */
#include <errno.h>

#define MAXPENDING 5    /* Maximum outstanding connection requests */
#define MAXPATHSIZE 512 /* Maximum size of root path */
#define MAXLINESIZE 512 /* Maximum size of line in conf */
#define MAXCMDS 64      /* Maximum number of additional commands */

void DieWithError(char *errorMessage);  /* Error handling function */
void HandleTCPClient(int clntSocket);   /* TCP client handling function */
void ChildExitSignalHandler();     /* Function to clean up zombie child processes */
int CreateTCPServerSocket(unsigned short port); /* Create TCP server socket */
int AcceptTCPConnection(int servSock);  /* Accept TCP connection request */



/* Global so accessable by SIGCHLD signal handler */
unsigned int childProcCount = 0;   /* Number of child processes */

int main(int argc, char *argv[])
{
    int servSock;                        /* Socket descriptor for server */
    int clntSock;                        /* Socket descriptor for client */
    unsigned short echoServPort;         /* Server port */
    pid_t processID;                     /* Process ID from fork() */
    struct sigaction myAction;           /* Signal handler specification structure */
    int port;                            /* Listening port */
    char line[MAXLINESIZE];              /* Buffer for reading config file */
    char rootPath[MAXPATHSIZE];          /* Root path */
    char commands[MAXCMDS][MAXLINESIZE]; /* List of additional commands */
 
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
    
    /* PORT */
    fgets(line, MAXLINESIZE, f);
    fgets(line, MAXLINESIZE, f);
    port = atoi(line);
    if(port < 1024 || port > 65535){
        fprintf(stderr, "Port provided is invalid\n");
        exit(1);
    }
    /* ROOT DIRECTORY */
    fgets(line, MAXLINESIZE, f);
    fgets(line, MAXLINESIZE, f);
    strcpy(rootPath, line);
    char* token = strtok(rootPath, "\t\n ");
    printf("token %s\n", token);
    printf("compare: %d\n", strcmp(token, "/tmp/local"));
    int res = access("/tmp", F_OK);
    printf("res %d\n", res);
    if(res != 0){
        /* Directory does not exist. */
        printf("token %s\n", rootPath);
        fprintf(stderr, "Root path does not exist\n");
        exit(1);
    }
    /* COMMANDS */
    fgets(line, MAXLINESIZE, f);
    for(int i = 0; i < MAXCMDS; i++){
        if(!fgets(line, MAXLINESIZE, f)) 
            break;
        strcpy(commands[i], line);
    }


    servSock = CreateTCPServerSocket(port);

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