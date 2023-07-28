#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h>     /* for close() */
#include <string.h>     /* for strcat() */

#define BUFFSIZE 16     /* Size of receive buffer */
#define MAXCMDSIZE 16   /* Max size of command received */

void DieWithError(char *errorMessage);  /* Error handling function */

void HandleTCPClient(int clntSocket)
{
    char buffer[BUFFSIZE];        /* Buffer for received message */
    int recvSize;                 /* Size of received message */
    char cmd[MAXCMDSIZE];         /* Buffer for whole command */
    int cmdSize;                  /* Size of whole command */     

    memset(cmd, 0, MAXCMDSIZE);   /* zero-ing cmd memory */

    while(1){
        cmd[0] = '\0';
        
        /* Receive message from client */
        if ((recvSize = recv(clntSocket, buffer, BUFFSIZE, 0)) < 0)
            DieWithError("recv() failed");

        buffer[recvSize] = '\0';
        strcat(cmd,  buffer);

        // /* Receive again until end of transmission */
        // while (recvCmdSize > 0)      /* zero indicates end of transmission */
        // {
        //     /* See if there is more data to receive */
        //     if ((recvCmdSize = recv(clntSocket, buffer, BUFFSIZE, 0)) < 0)
        //         DieWithError("recv() failed");
        //     cmdSize += recvCmdSize;
        // }

        printf("Received %s\n", cmd);
        cmdSize = strlen(cmd);

        /* Echo message back to client */
        if (send(clntSocket, cmd, cmdSize, 0) != cmdSize)
            DieWithError("send() failed");
        
        if(strcmp(cmd, "exit") == 0) /* Check if exit is typed */
            break;
    }

    close(clntSocket);    /* Close client socket */
}
