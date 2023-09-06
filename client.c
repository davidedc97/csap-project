#include <stdio.h>          /* for printf() and fprintf() */
#include <sys/socket.h>     /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>      /* for sockaddr_in and inet_addr() */
#include <stdlib.h>         /* for atoi() and exit() */
#include <string.h>         /* for memset() */
#include <unistd.h>         /* for close() and getuid() */
#include <sys/types.h>      /* for getuid() */
#include <pwd.h>            /* for getpwuid() */
#include <ctype.h>          /* for isspace() */

#define BUFFSIZE 1024       /* Size of buffer for received message */
#define MAXCMDSIZE 1024     /* Max size for command to send */
#define MAXUSRSIZE 1024     /* Max size for username and password */

void DieWithError(char *errorMessage);                  /* Error handling function */
int Login(int socket, char* username, char* password);  /* Get user and pw to send to server */
void GetWelcomeMessage(int socket);                     /* Displays a welcome message */
char* strstrip(char *s);                                /* Strip spaces of a string */

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
    int bytesRcvd;                   /* Bytes read in single recv() */
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

    int logged = Login(sock, username, password);
    printf("Result: %d\n", logged);

    if (logged != 0){
        printf("Bad credentials\n");
        close(sock);
        exit(1);
    }
    GetWelcomeMessage(sock);

    while(1){
        int redirectToFile = 0;
        char* outputFile = (char*)malloc(MAXCMDSIZE);

        fgets(cmd, MAXCMDSIZE, stdin); /* Read command from stdin */
        cmdLen = strlen(cmd);
        cmd[cmdLen - 1] = '\0'; /* fgets() does not automatically discard new line */
        
        /* Send the command to the server */
        if (send(sock, cmd, cmdLen, 0) != cmdLen)
            DieWithError("send() sent a different number of bytes than expected");

        /* Check if exit is typed */
        if(strcmp(cmd, "exit") == 0)
            break;
        
        /* Check if run is typed and if there's a redirection to do on client side */
        char s[4] = " \t\n";
        char* tmpCmd = (char*)malloc(MAXCMDSIZE); // cmd to tokenize 
        strcpy(tmpCmd, cmd);
        char* token = strtok(tmpCmd, s);
        // printf("Client Token: <%s>\n", token);
        if(strcmp(token, "run") == 0){
            char* redirectionChar = strchr(cmd, '>');
            if(redirectionChar != NULL) {
                /* If there's a redirection, it must be outside quotes " " */
                char* afterRedirection = redirectionChar + 1;
                char* firstQuote = strchr(afterRedirection, '\"');
                if(firstQuote == NULL){
                    redirectToFile = 1;
                    token = strtok(NULL, ">"); // command before >
                    token = strtok(NULL, ">"); // command after >, the destination of the redirection
                    // printf("Found token of redirection: <%s>\n", token);
                    strcpy(outputFile, token);
                    outputFile = strstrip(outputFile);
                }
            }
        }
        free(tmpCmd);
        

        /* Get response from the server */
        bytesRcvd = 0;
        // printf("Received: ");

        /* Check if output should be redirected to a file or displayed on screen */
        if(redirectToFile){
            FILE* fpt = fopen(outputFile, "w");
            if(fpt == NULL) {
                printf("Cannot open file %s\n", outputFile);
                close(sock);
                exit(0);
            }
            printf("Writing to %s... ", outputFile);
            do {
                buffer[bytesRcvd] = '\0';
                fprintf(fpt, "%s\n", buffer);
            } while((bytesRcvd = recv(sock, buffer, BUFFSIZE-1, 0))>=BUFFSIZE-1);
            buffer[bytesRcvd] = '\0';
            fprintf(fpt, "%s\n", buffer);
            fclose(fpt);
            printf("Done\n");
        }
        else{
            do {
                buffer[bytesRcvd] = '\0';
                printf("%s\n", buffer);
            } while((bytesRcvd = recv(sock, buffer, BUFFSIZE-1, 0))>=BUFFSIZE-1);
            buffer[bytesRcvd] = '\0';
            printf("%s\n", buffer);
        }
        
    }


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
    
    sprintf(buffer, "%s\n%s", username, password);
    if (send(socket, buffer, strlen(buffer), 0) != strlen(buffer))
            DieWithError("send() sent a different number of bytes than expected");
    
    /* Get response */
    printf("listening\n");
    bytesRcvd = recv(socket, buffer, BUFFSIZE-1, 0);
    printf("bytesreceived: %d\n", bytesRcvd);
    buffer[bytesRcvd] = '\0';
    printf("msg received: %s\n", buffer);
    int res = atoi(buffer);
    return res;
}
void GetWelcomeMessage(int socket){
    char buffer[BUFFSIZE];
    int bytesRcvd = 0;

    bytesRcvd = recv(socket, buffer, BUFFSIZE-1, 0);
    buffer[bytesRcvd] = '\0';
    printf("%s\n", buffer);
}

char* strstrip(char *s) {
    size_t size;
    char *end;
    size = strlen(s);

    if (!size)
            return s;
    end = s + size - 1;
    while (end >= s && isspace(*end))
            end--;
    *(end + 1) = '\0';

    while (*s && isspace(*s))
            s++;
    return s;
}
