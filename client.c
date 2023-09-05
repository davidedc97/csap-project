#include <stdio.h>          /* for printf() and fprintf() */
#include <sys/socket.h>     /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>      /* for sockaddr_in and inet_addr() */
#include <stdlib.h>         /* for atoi() and exit() */
#include <string.h>         /* for memset() */
#include <unistd.h>         /* for close() and getuid() */
#include <sys/types.h>      /* for getuid() */
#include <pwd.h>            /* for getpwuid() */

#define BUFFSIZE 1024       /* Size of buffer for received message */
#define MAXCMDSIZE 1024     /* Max size for command to send */
#define MAXUSRSIZE 1024     /* Max size for username and password */

void DieWithError(char *errorMessage);                  /* Error handling function */
int Login(int socket, char* username, char* password);  /* Get user and pw to send to server */

int main(int argc, char *argv[])
{
    int sock;                        /* Socket descriptor */
    struct sockaddr_in servAddr;     /* Server address */
    unsigned short servPort;         /* Server port */
    char *servIP;                    /* Server IP address (dotted quad) */
    char cmd[MAXCMDSIZE];            /* Buffer for command to send */
    unsigned int cmdLen;             /* Length of command to send */
    char buffer[BUFFSIZE];           /* Buffer for received message */
    unsigned int buffLen;            /* Length of received message */
    int bytesRcvd, totalBytesRcvd;   /* Bytes read in single recv() and total bytes read */
    char username[MAXUSRSIZE];       /* Username of the client */
    char password[MAXUSRSIZE];       /* Password of the client */



    if ((argc < 3) || (argc > 3))    /* Test for correct number of arguments */
    {
       fprintf(stderr, "Usage: %s <Server IP> <Port>\n",
               argv[0]);
       exit(1);
    }

    servIP = argv[1];              /* First arg: server IP address (dotted quad) */
    servPort = atoi(argv[2]);  /* Second arg: remote port to connect */


    /* Create a reliable, stream socket using TCP */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    /* Construct the server address structure */
    memset(&servAddr, 0, sizeof(servAddr));         /* Zero out structure */
    servAddr.sin_family      = AF_INET;             /* Internet address family */
    servAddr.sin_addr.s_addr = inet_addr(servIP);   /* Server IP address */
    servAddr.sin_port        = htons(servPort);     /* Server port */

    /* Establish the connection to the server */
    if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
        DieWithError("connect() failed");

    // int res = Login(sock, username, password);


    totalBytesRcvd = 0;
    bytesRcvd = 0;
    printf("Received: ");
    
    do {
        buffer[bytesRcvd] = '\0';
        printf("%s\n", buffer);
    } while((bytesRcvd = recv(sock, buffer, BUFFSIZE-1, 0))>=BUFFSIZE-1);
    buffer[bytesRcvd] = '\0';
    printf("%s\n", buffer);

    while(1){
        fgets(cmd, MAXCMDSIZE, stdin); /* Read command from stdin */
        cmdLen = strlen(cmd);
        cmd[cmdLen - 1] = '\0'; /* fgets() does not automatically discard new line */
        
        /* Send the command to the server */
        if (send(sock, cmd, cmdLen, 0) != cmdLen)
            DieWithError("send() sent a different number of bytes than expected");
        if(strcmp(cmd, "exit") == 0) /* Check if exit is typed */
            break;

        /* Receive the same string back from the server */
        totalBytesRcvd = 0;
        bytesRcvd = 0;
        printf("Received: ");

        do {
            buffer[bytesRcvd] = '\0';
            printf("%s\n", buffer);
        } while((bytesRcvd = recv(sock, buffer, BUFFSIZE-1, 0))>=BUFFSIZE-1);
        buffer[bytesRcvd] = '\0';
        printf("%s\n", buffer);
    }

  

  

    printf("\n");    /* Print a final linefeed */
    printf("Closing connection\n");

    close(sock);
    exit(0);
}



int Login(int socket, char* username, char* password) {
    struct passwd *p = getpwuid(getuid());  // Check for NULL!
    char buffer[BUFFSIZE];
    int bytesRcvd = 0;

    strcpy(username, p->pw_name);
    printf("Password for user %s:\n", username);
    password = getpass("");
    printf("User: %s\n", username);
    printf("Password: %s\n", password);
    
    sprintf(buffer, "%s\n%s", username, password);
    if (send(socket, buffer, strlen(buffer), 0) != strlen(buffer))
            DieWithError("send() sent a different number of bytes than expected");

    /* Get response */
    bytesRcvd = recv(socket, buffer, BUFFSIZE-1, 0);
    buffer[bytesRcvd] = '\0';
    printf("Received from login: %s\n", buffer);
    int res = atoi(buffer);
    printf("returning %d\n", res);
    return 0;
}
