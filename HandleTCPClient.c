#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h>     /* for close() */
#include <string.h>     /* for strcat() */
#include <stdlib.h>     /* for popen() */
#include <sys/wait.h>   /* for waitpid() */
#include <ctype.h>


#define BUFFSIZE 1024     /* Size of receive buffer */
#define MAXCMDSIZE 1024   /* Max size of command received */
#define MAXARGS 100       /* Max number of arguments for a command */
#define MAXUSRSIZE 1024   /* Max size for username and password */
#define MAXCMDS 64        /* Maximum number of additional commands */


void DieWithError(char *errorMessage);                      /* Error handling function */
void DisplayWelcome(int clntSocket, char* cwd);             /* Display a welcome message */
int Login(int clntSocket, char* username, char* password);  /* Check credentials */
char* strstrip(char *s);



void HandleTCPClient(int clntSocket, char** commands)
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
    
    // Login(clntSocket, username, password);
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
                char* pch2 = strchr(pch+1, '"'); // get pointer to second "
                if(pch2 != NULL){
                    /* Check if every command is in the allowed commands */
                    token = strtok(NULL, "\"");
                    char insideCmd[MAXCMDSIZE];
                    char tmpCmd[MAXCMDSIZE];
                    char* tokenizedCmds[MAXCMDS];
                    int badSyntax = 0;
                    strcpy(insideCmd, token);
                    strcpy(tmpCmd, token);
                    printf("insideCmd: %s\n", insideCmd);

                    /* If there's a | , check only the first token of each command to see if it's allowed */
                    if(strchr(tmpCmd, '|') != NULL){
                        printf("entro qui\n");
                        int cmdCounter = 0;
                        /* grazie al cazzo, devo fare un do while qua sotto per il token, non passo mai NULL */
                        while((token = strtok(tmpCmd, "|")) != NULL){
                            printf("almeno uno lo faccio\n");
                            tokenizedCmds[cmdCounter++] = token;
                        }
                        for(int tmp=0; tmp < cmdCounter; tmp++){
                            printf("commands in pipe: <%s>\n", tokenizedCmds[cmdCounter]);
                        }

                        char* firstPipe = strchr(tmpCmd, '|');
                        /* Only one pipe allowed */
                        char* cmdPipe = strtok(tmpCmd, "|");
                        char* cmd2 = strtok(NULL, "|");
                        // printf("found cmdPipe %s\n", cmdPipe);

                        do {
                            printf("found cmdPipe <%s>\n", cmdPipe);
                            // char* tknPipe = strstrip(strtok(NULL, " "));
                            char* tknPipe = strtok(NULL, " ");
                            printf("tknPipe: <%s>\n", tknPipe);
                            // char* trimmed = strstrip(tknPipe);
                            // printf("Trimmed: <%s>\n", trimmed);
                            int foundCmd = 0;
                            if(tknPipe != NULL){
                                printf("qua dentro");
                                for(int mc = 0; mc < MAXCMDS; mc++){
                                    printf("Comparing %s to %s\n", tknPipe, commands[mc]);
                                    if(strcmp(commands[mc], tknPipe) == 0){
                                        foundCmd = 1;
                                        break;
                                    }
                                }
                            }
                            else {
                                badSyntax = 1;
                            }
                            if(!foundCmd)
                                badSyntax = 1;
                        } while ((cmdPipe = strtok(NULL, "|")) != NULL && !badSyntax);
                       printf("fuori\n");
                        
                        
                        // // Checking first token of the command 1
                        // char* tkn1 = strtok(cmd1, s);
                        // int foundCmd1 = 0;
                        // if(tkn1 != NULL){
                        //     for(int mc = 0; mc < MAXCMDS; mc++){
                        //         printf("Comparing %s to %s\n", tkn1, commands[mc]);
                        //         if(strcmp(commands[mc], tkn1) == 0){
                        //             foundCmd1 = 1;
                        //             break;
                        //         }
                        //     }
                        // }
                        // else {
                        //     badSyntax = 1;
                        // }
                        
                        // // Checking first token of the command 2
                        // char* tkn2 = strtok(cmd2, s);
                        // int foundCmd2 = 0;
                        // if(tkn2 != NULL){
                        //     for(int mc = 0; mc < MAXCMDS; mc++){
                        //         printf("Comparing %s to %s\n", tkn2, commands[mc]);
                        //         if(strcmp(commands[mc], tkn2) == 0){
                        //             foundCmd2 = 1;
                        //             break;
                        //         }
                        //     }
                        // }
                        // else {
                        //     badSyntax = 1;
                        // }
                        
                        
                        // if(!foundCmd1 || !foundCmd2)
                        //     badSyntax = 1;
                            
                    }
                    // while (token != NULL) {
                    //     printf("TOKEN: %s\n", token);
                    //     if(strcmp(token, "|") == 0)
                    //         continue;
                    //     if(strcmp(token, ">") == 0)
                    //         break;
                    //     int found = 0;
                    //     printf("arrivo qua\n");
                    //     for(int mc = 0; mc < MAXCMDS; mc++){
                    //         printf("Comparing %s to %s\n", token, commands[mc]);
                    //         if(strcmp(commands[mc], token) == 0){
                    //             found = 1;
                    //             break;
                    //         }
                    //     }
                    //     if(!found){
                    //         /* Command not allowed */
                    //         printf("Command not allowed\n");
                    //         break;
                    //     }
                    //     token = strtok(NULL, s);
                    // }
                    argv[2] = tmpCmd;
                }
                else{
                    // BAD SYNTAX
                    printf("Bad syntax\n");
                }
                
            }
            /* If no " " are present we take only the first argument as cmd */
            /* If there is a redirection, it is on client-side */
            else {
                token = strtok(NULL, s);
                argv[2] = token;
            }
            
            
            /* Last element in argv should be NULL from documentation */
            argv[3] = NULL;
            
            for(int k=0; k<MAXARGS; k++){
                printf("Elem %d: %s\n", k, argv[k]);
                if(argv[k] == NULL)
                    break;
            }
            continue;

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


