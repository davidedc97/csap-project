#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h>     /* for close() */
#include <string.h>     /* for strcat() */
#include <stdlib.h>     /* for popen() */
#include <sys/wait.h>   /* for waitpid() */


#define BUFFSIZE 1024     /* Size of receive buffer */
#define MAXCMDSIZE 1024   /* Max size of command received */
#define MAXARGS 100       /* Max number of arguments for a command */
#define MAXUSRSIZE 1024     /* Max size for username and password */


void DieWithError(char *errorMessage);                      /* Error handling function */
void DisplayWelcome(int clntSocket, char* cwd);             /* Display a welcome message */
int Login(int clntSocket, char* username, char* password);  /* Check credentials */



void HandleTCPClient(int clntSocket)
{
    char buffer[BUFFSIZE];        /* Buffer for received message */
    int recvSize;                 /* Size of received message */
    char cmd[MAXCMDSIZE];         /* Buffer for whole command */
    char cmdOrig[MAXCMDSIZE];     /* Backup not meant to be modified by tokenizer */
    int cmdSize;                  /* Size of whole command */     
    char output[BUFFSIZE];        /* Buffer for output of cmd */
    char cwd[BUFFSIZE];           /* Buffer for storing the current working directory */
    char username[MAXUSRSIZE];    /* Username of the client */
    char password[MAXUSRSIZE];    /* Password of the client */
    FILE *fp;                     /* Pointer for popen fd */


    memset(buffer, 0, BUFFSIZE);   /* zero-ing buffer memory */
    memset(cmd, 0, MAXCMDSIZE);    /* zero-ing cmd memory */
    memset(cwd, 0, BUFFSIZE);      /* zero-ing cwd memory */

    if (getcwd(cwd, sizeof(cwd)) == NULL)
        DieWithError("getcwd() failed");
    
    Login(clntSocket, username, password);
    DisplayWelcome(clntSocket, cwd);

    while(1){
        cmd[0] = '\0';
        
        /* Receive message from client */
        if ((recvSize = recv(clntSocket, buffer, BUFFSIZE, 0)) < 0)
            DieWithError("recv() failed");

        buffer[recvSize] = '\0';
        strcat(cmd, buffer);
        strcpy(cmdOrig, cmd);

        printf("Received: %s\n", cmd);
        cmdSize = strlen(cmd);

       

        /* Tokenize received command */
        const char s[4] = " \t\n";
        char *token;
        token = strtok(cmd, s); /* First token */

        printf("Token: %s\n", token);

        /* "Switch case" on command */

        if(strcmp(cmd, "copy") == 0) {
            int pipefd[2];
            char* argv[MAXARGS];
            memset(argv, 0, MAXARGS);
            argv[0] = "/bin/cp";
            int i = 1;

            /* Get command args */
            while ((token = strtok(NULL, s)) != NULL) {
                if(i >= MAXARGS) 
                    break;
                argv[i++] = token;
            }
            /* Last element in argv should be NULL from documentation */
            if(i >= MAXARGS)
                argv[MAXARGS] = NULL;
            else
                argv[i] = NULL;
            
            for(int k=0; k<MAXARGS; k++){
                printf("Elem %d: %s\n", k, argv[k]);
                if(argv[k] == NULL)
                    break;
            }

            /* pipe() to read the output of the command */
            if (pipe(pipefd) == -1)
                DieWithError("pipe() failed");

            /* Fork() + exec() to execute the command */
            pid_t pid = fork();

            if (pid == -1)
                DieWithError("fork() failed");
            else if (pid > 0) {
                /* Inside parent */
                close(pipefd[1]);
                int nbytes = 0;
                memset(output, 0, BUFFSIZE);
                memset(buffer, 0, BUFFSIZE);
                output[0] = '\0';
                buffer[0] = '\0';

                while((nbytes = read(pipefd[0], buffer, sizeof(buffer))) != 0) {
                    strcat(output, buffer);
                    memset(buffer, 0, BUFFSIZE);
                }

                printf("Output: %s\n", output);
                int status;
                waitpid(pid, &status, 0);
                printf("Exit status: %d\n", status);

                /* Upon successful completion, cp shall return 0 */
                if(status != 0) {
                    if (send(clntSocket, output, strlen(output), 0) != strlen(output))
                        DieWithError("send() failed");
                }
                else {
                    const char* msg = "copy successful";
                    if (send(clntSocket, msg, strlen(msg), 0) != strlen(msg))
                        DieWithError("send() failed");
                }
            }
            else {
                /* Inside child */
                dup2 (pipefd[1], STDOUT_FILENO);
                dup2 (pipefd[1], STDERR_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);
                if (execve(argv[0], argv, NULL) == -1)  /* execve returns only in case of error */
                    DieWithError("Could not execve()");
            }
        }
        else if(strcmp(cmd, "move") == 0) {
            int pipefd[2];
            char* argv[MAXARGS];
            memset(argv, 0, MAXARGS);
            argv[0] = "/bin/mv";
            int i = 1;

            /* Get command args */
            while ((token = strtok(NULL, s)) != NULL) {
                if(i >= MAXARGS) 
                    break;
                argv[i++] = token;
            }
            /* Last element in argv should be NULL from documentation */
            if(i >= MAXARGS)
                argv[MAXARGS] = NULL;
            else
                argv[i] = NULL;
            
            for(int k=0; k<MAXARGS; k++){
                printf("Elem %d: %s\n", k, argv[k]);
                if(argv[k] == NULL)
                    break;
            }

            /* pipe() to read the output of the command */
            if (pipe(pipefd) == -1)
                DieWithError("pipe() failed");

            /* Fork() + exec() to execute the command */
            pid_t pid = fork();

            if (pid == -1)
                DieWithError("fork() failed");
            else if (pid > 0) {
                /* Inside parent */
                close(pipefd[1]);
                int nbytes = 0;
                memset(output, 0, BUFFSIZE);
                memset(buffer, 0, BUFFSIZE);
                output[0] = '\0';
                buffer[0] = '\0';

                while((nbytes = read(pipefd[0], buffer, sizeof(buffer))) != 0) {
                    strcat(output, buffer);
                    memset(buffer, 0, BUFFSIZE);
                }

                printf("Output: %s\n", output);
                int status;
                waitpid(pid, &status, 0);
                printf("Exit status: %d\n", status);

                /* Upon successful completion, mv shall return 0 */
                if(status != 0) {
                    if (send(clntSocket, output, strlen(output), 0) != strlen(output))
                        DieWithError("send() failed");
                }
                else {
                    const char* msg = "move successful";
                    if (send(clntSocket, msg, strlen(msg), 0) != strlen(msg))
                        DieWithError("send() failed");
                }
            }
            else {
                /* Inside child */
                dup2 (pipefd[1], STDOUT_FILENO);
                dup2 (pipefd[1], STDERR_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);
                if (execve(argv[0], argv, NULL) == -1)  /* execve returns only in case of error */
                    DieWithError("Could not execve()");
            }
        }
        else if(strcmp(cmd, "delete") == 0) {
            int pipefd[2];
            char* argv[MAXARGS];
            argv[0] = "/bin/rm";
            argv[1] = "-f";
            int i = 2;

            /* Get command args */
            while ((token = strtok(NULL, s)) != NULL) {
                if(i >= MAXARGS) 
                    break;
                argv[i++] = token;
            }            
            
            for(int k=0; k<MAXARGS; k++){
                printf("Elem %d: %s\n", k, argv[k]);
                if(argv[k] == NULL)
                    break;
            }

            /* pipe() to read the output of the command */
            if (pipe(pipefd) == -1)
                DieWithError("pipe() failed");

            /* Fork() + exec() to execute the command */
            pid_t pid = fork();

            if (pid == -1)
                DieWithError("fork() failed");
            else if (pid > 0) {
                /* Inside parent */
                close(pipefd[1]);
                int nbytes = 0;
                memset(output, 0, BUFFSIZE);
                memset(buffer, 0, BUFFSIZE);
                output[0] = '\0';
                buffer[0] = '\0';

                while((nbytes = read(pipefd[0], buffer, sizeof(buffer))) != 0) {
                    strcat(output, buffer);
                    memset(buffer, 0, BUFFSIZE);
                }

                printf("Output: %s\n", output);
                int status;
                waitpid(pid, &status, 0);
                printf("Exit status: %d\n", status);

                /* Upon successful completion, rm shall return 0*/
                if(status != 0) {
                    if (send(clntSocket, output, strlen(output), 0) != strlen(output))
                        DieWithError("send() failed");
                }
                else {
                    const char* msg = "delete successful";
                    if (send(clntSocket, msg, strlen(msg), 0) != strlen(msg))
                        DieWithError("send() failed");
                }
            }
            else {
                /* Inside child */
                dup2 (pipefd[1], STDOUT_FILENO);
                dup2 (pipefd[1], STDERR_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);
                if (execve(argv[0], argv, NULL) == -1)  /* execve returns only in case of error */
                    DieWithError("Could not execve()");
            }
        }
        else if(strcmp(cmd, "list") == 0) {
            int pipefd[2];
            char* argv[MAXARGS];
            argv[0] = "/bin/ls";
            argv[1] = "-l";
            argv[2] = NULL;
            
            for(int k=0; k<MAXARGS; k++){
                printf("Elem %d: %s\n", k, argv[k]);
                if(argv[k] == NULL)
                    break;
            }

            /* pipe() to read the output of the command */
            if (pipe(pipefd) == -1)
                DieWithError("pipe() failed");

            /* Fork() + exec() to execute the command */
            pid_t pid = fork();

            if (pid == -1)
                DieWithError("fork() failed");
            else if (pid > 0) {
                /* Inside parent */
                close(pipefd[1]);
                int nbytes = 0;
                memset(output, 0, BUFFSIZE);
                memset(buffer, 0, BUFFSIZE);
                output[0] = '\0';
                buffer[0] = '\0';

                while((nbytes = read(pipefd[0], buffer, sizeof(buffer))) != 0) {
                    strcat(output, buffer);
                    memset(buffer, 0, BUFFSIZE);
                }

                printf("Output: %s\n", output);
                int status;
                waitpid(pid, &status, 0);
                printf("Exit status: %d\n", status);

                /* Upon successful completion, ls shall return 0*/
                // TODO: no ripetizioni
                if(status != 0) {
                    if (send(clntSocket, output, strlen(output), 0) != strlen(output))
                        DieWithError("send() failed");
                }
                else {
                    if (send(clntSocket, output, strlen(output), 0) != strlen(output))
                        DieWithError("send() failed");
                }
            }
            else {
                /* Inside child */
                dup2 (pipefd[1], STDOUT_FILENO);
                dup2 (pipefd[1], STDERR_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);
                if (execve(argv[0], argv, NULL) == -1)  /* execve returns only in case of error */
                    DieWithError("Could not execve()");
            }
        }
        else if(strcmp(cmd, "create_dir") == 0) {
            int pipefd[2];
            char* argv[MAXARGS];
            memset(argv, 0, MAXARGS);
            argv[0] = "/bin/mkdir";
            int i = 1;

            /* Get command args */
            while ((token = strtok(NULL, s)) != NULL) {
                if(i >= MAXARGS) 
                    break;
                argv[i++] = token;
            }
            /* Last element in argv should be NULL from documentation */
            if(i >= MAXARGS)
                argv[MAXARGS] = NULL;
            else
                argv[i] = NULL;
            
            for(int k=0; k<MAXARGS; k++){
                printf("Elem %d: %s\n", k, argv[k]);
                if(argv[k] == NULL)
                    break;
            }

            /* pipe() to read the output of the command */
            if (pipe(pipefd) == -1)
                DieWithError("pipe() failed");

            /* Fork() + exec() to execute the command */
            pid_t pid = fork();

            if (pid == -1)
                DieWithError("fork() failed");
            else if (pid > 0) {
                /* Inside parent */
                close(pipefd[1]);
                int nbytes = 0;
                memset(output, 0, BUFFSIZE);
                memset(buffer, 0, BUFFSIZE);
                output[0] = '\0';
                buffer[0] = '\0';

                while((nbytes = read(pipefd[0], buffer, sizeof(buffer))) != 0) {
                    strcat(output, buffer);
                    memset(buffer, 0, BUFFSIZE);
                }

                printf("Output: %s\n", output);
                int status;
                waitpid(pid, &status, 0);
                printf("Exit status: %d\n", status);

                /* Upon successful completion, mkdir shall return 0 */
                if(status != 0) {
                    if (send(clntSocket, output, strlen(output), 0) != strlen(output))
                        DieWithError("send() failed");
                }
                else {
                    const char* msg = "create_dir successful";
                    if (send(clntSocket, msg, strlen(msg), 0) != strlen(msg))
                        DieWithError("send() failed");
                }
            }
            else {
                /* Inside child */
                dup2 (pipefd[1], STDOUT_FILENO);
                dup2 (pipefd[1], STDERR_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);
                if (execve(argv[0], argv, NULL) == -1)  /* execve returns only in case of error */
                    DieWithError("Could not execve()");
            }

        }
        else if(strcmp(cmd, "delete_dir") == 0) {
            int pipefd[2];
            char* argv[MAXARGS];
            argv[0] = "/bin/rm";
            argv[1] = "-d";
            int i = 2;

            /* Get command args */
            while ((token = strtok(NULL, s)) != NULL) {
                if(i >= MAXARGS) 
                    break;
                argv[i++] = token;
            }            
            
            for(int k=0; k<MAXARGS; k++){
                printf("Elem %d: %s\n", k, argv[k]);
                if(argv[k] == NULL)
                    break;
            }

            /* pipe() to read the output of the command */
            if (pipe(pipefd) == -1)
                DieWithError("pipe() failed");

            /* Fork() + exec() to execute the command */
            pid_t pid = fork();

            if (pid == -1)
                DieWithError("fork() failed");
            else if (pid > 0) {
                /* Inside parent */
                close(pipefd[1]);
                int nbytes = 0;
                memset(output, 0, BUFFSIZE);
                memset(buffer, 0, BUFFSIZE);
                output[0] = '\0';
                buffer[0] = '\0';

                while((nbytes = read(pipefd[0], buffer, sizeof(buffer))) != 0) {
                    strcat(output, buffer);
                    memset(buffer, 0, BUFFSIZE);
                }

                printf("Output: %s\n", output);
                int status;
                waitpid(pid, &status, 0);
                printf("Exit status: %d\n", status);

                /* Upon successful completion, rm shall return 0*/
                if(status != 0) {
                    if (send(clntSocket, output, strlen(output), 0) != strlen(output))
                        DieWithError("send() failed");
                }
                else {
                    const char* msg = "delete_dir successful";
                    if (send(clntSocket, msg, strlen(msg), 0) != strlen(msg))
                        DieWithError("send() failed");
                }
            }
            else {
                /* Inside child */
                dup2 (pipefd[1], STDOUT_FILENO);
                dup2 (pipefd[1], STDERR_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);
                if (execve(argv[0], argv, NULL) == -1)  /* execve returns only in case of error */
                    DieWithError("Could not execve()");
            }
        }
        else if(strcmp(cmd, "cd") == 0) {
            int pipefd[2];
            char* argv[MAXARGS];
            memset(argv, 0, MAXARGS);
            argv[0] = "cd";
            int i = 1;

            /* Get only next arg */
            if ((token = strtok(NULL, s)) != NULL)
                argv[i++] = token;
            
            /* Last element in argv should be NULL from documentation */
            if(i >= MAXARGS)
                argv[MAXARGS] = NULL;
            else
                argv[i] = NULL;
            
            for(int k=0; k<MAXARGS; k++){
                printf("Elem %d: %s\n", k, argv[k]);
                if(argv[k] == NULL)
                    break;
            }

            /* Error */
            if (chdir(argv[1]) != 0){
                char* msg = "chdir() failed";
                if (send(clntSocket, msg, strlen(msg), 0) != strlen(msg))
                    DieWithError("send() failed");
            }
            else {
                if (getcwd(cwd, sizeof(cwd)) == NULL)
                    DieWithError("getcwd() failed");

                char msg[BUFFSIZE * 2] = "Current path:\n";
                strcat(msg, cwd);
                if (send(clntSocket, msg, strlen(msg), 0) != strlen(msg))
                    DieWithError("send() failed");
            }

        }
        else if(strcmp(cmd, "run") == 0) {
            int pipefd[2];
            char* argv[MAXARGS];
            memset(argv, 0, MAXARGS);
            argv[0] = "/bin/bash";
            argv[1] = "-c";
            int i = 1;

            /* Check if args are between " " */
            char* pch;
            if((pch = strchr(cmdOrig, '"')) != NULL){
                char* pch2;
                char* pch3 = strchr(pch+1, '"'); // get pointer to second "
                if(pch3 == NULL){
                    // BAD SYNTAX
                    printf("Bad syntax\n");
                }
                token = strtok(NULL, "\"");
                argv[2] = token;
                // /* Check if the command has a pipe INSIDE " "*/
                // if((pch2 = strchr(cmdOrig, '|')) != NULL){
                //     char * pch3 = strchr(pch+1, '"');
                // }
                // /* Check if there's a redirection INSIDE " " */
                // if((pch2 = strchr(cmdOrig, '>')) != NULL){
                //     /* Check if the redirection is before last " */
                //     int index = (int)pch2 - pch3;
                //     if()
                    
                // }
            }
            else if((pch = strchr(cmdOrig, '>')) != NULL){

            }
            
            /* Get command args */
            while ((token = strtok(NULL, s)) != NULL) {
                if(i >= MAXARGS) 
                    break;
                argv[i++] = token;
            }
            /* Last element in argv should be NULL from documentation */
            if(i >= MAXARGS)
                argv[MAXARGS] = NULL;
            else
                argv[i] = NULL;
            
            for(int k=0; k<MAXARGS; k++){
                printf("Elem %d: %s\n", k, argv[k]);
                if(argv[k] == NULL)
                    break;
            }

            /* pipe() to read the output of the command */
            if (pipe(pipefd) == -1)
                DieWithError("pipe() failed");

            /* Fork() + exec() to execute the command */
            pid_t pid = fork();

            if (pid == -1)
                DieWithError("fork() failed");
            else if (pid > 0) {
                /* Inside parent */
                close(pipefd[1]);
                int nbytes = 0;
                memset(output, 0, BUFFSIZE);
                memset(buffer, 0, BUFFSIZE);
                output[0] = '\0';
                buffer[0] = '\0';

                while((nbytes = read(pipefd[0], buffer, sizeof(buffer))) != 0) {
                    strcat(output, buffer);
                    memset(buffer, 0, BUFFSIZE);
                }

                printf("Output: %s\n", output);
                int status;
                waitpid(pid, &status, 0);
                printf("Exit status: %d\n", status);

                /* Upon successful completion, mkdir shall return 0 */
                if(status != 0) {
                    if (send(clntSocket, output, strlen(output), 0) != strlen(output))
                        DieWithError("send() failed");
                }
                else {
                    const char* msg = "create_dir successful";
                    if (send(clntSocket, msg, strlen(msg), 0) != strlen(msg))
                        DieWithError("send() failed");
                }
            }
            else {
                /* Inside child */
                dup2 (pipefd[1], STDOUT_FILENO);
                dup2 (pipefd[1], STDERR_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);
                if (execve(argv[0], argv, NULL) == -1)  /* execve returns only in case of error */
                    DieWithError("Could not execve()");
            }
        }
        else if(strcmp(cmd, "exit") == 0) {
            printf("Closing connection with %d\n", getpid());
            break;
        }
        else{
            const char* msg = "Invalid command";
            if (send(clntSocket, msg, strlen(msg), 0) != strlen(msg))
                DieWithError("send() failed");
        }


    }

    close(clntSocket);    /* Close client socket */
}


int Login(int clntSocket, char* username, char* password) {
    int recvBytes = 0;
    char buffer[BUFFSIZE];
    char command[MAXCMDSIZE];
    char output[BUFFSIZE]; 

    if ((recvBytes = recv(clntSocket, buffer, BUFFSIZE, 0)) < 0)
        DieWithError("recv() failed");
    
    char* token = strtok(buffer, "\n");
    strcpy(username, token);
    token = strtok(NULL, "\n");
    strcpy(password, token);

    printf("User: %s\n", username);
    printf("Password: %s\n", password);

    sprintf(command, "echo %s | su %s", password, username);
    // FILE* pipe = popen(command, "r");

    // int res = pclose(pipe);
    // printf("res of pclose: %d\n", res);
    // if (send(clntSocket, &res, sizeof(res), 0) != sizeof(res))
    //     DieWithError("send() failed");



    int pipefd[2];
    char* argv[MAXARGS];
    argv[0] = "/bin/bash";
    argv[1] = "-c";
    argv[2] = command;
    argv[3] = NULL;

    /* pipe() to read the output of the command */
    if (pipe(pipefd) == -1)
        DieWithError("pipe() failed");

    /* Fork() + exec() to execute the command */
    pid_t pid = fork();

    if (pid == -1)
        DieWithError("fork() failed");
    else if (pid > 0) {
        /* Inside parent */
        close(pipefd[1]);
        int nbytes = 0;
        memset(output, 0, BUFFSIZE);
        memset(buffer, 0, BUFFSIZE);
        output[0] = '\0';
        buffer[0] = '\0';

        while((nbytes = read(pipefd[0], buffer, sizeof(buffer))) != 0) {
            strcat(output, buffer);
            memset(buffer, 0, BUFFSIZE);
        }

        printf("Output: %s\n", output);
        int status;
        waitpid(pid, &status, 0);
        printf("Exit status: %d\n", status);

        /* Upon successful completion, su shall return 0*/
        // if(status != 0) {
        //     if (send(clntSocket, output, strlen(output), 0) != strlen(output))
        //         DieWithError("send() failed");
        // }
        // else {
        //     const char* msg = "delete_dir successful";
        //     if (send(clntSocket, msg, strlen(msg), 0) != strlen(msg))
        //         DieWithError("send() failed");
        // }
    }
    else {
        /* Inside child */
        dup2 (pipefd[1], STDOUT_FILENO);
        dup2 (pipefd[1], STDERR_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        if (execve(argv[0], argv, NULL) == -1)  /* execve returns only in case of error */
            DieWithError("Could not execve()");
    }
}

void DisplayWelcome(int clntSocket, char* cwd) {
    char welcomeMessage[BUFFSIZE * 2] = "Connected. Current path is\n"; /* Must be able to contain a path of size BUFFSIZE */
    strcat(welcomeMessage, cwd);
    printf("welcome message: %s\n", welcomeMessage);
    if (send(clntSocket, welcomeMessage, strlen(welcomeMessage), 0) != strlen(welcomeMessage))
        DieWithError("send() failed");
}



