#include <stdio.h>          /* for printf() and fprintf() */
#include <stdlib.h>         /* for atoi() and exit() */
#include <string.h>         /* for memset() */
#include <unistd.h>         /* for close() and getuid() */
#include <sys/wait.h>

#define MAXARGS 512

int main(){
    // int pipefd[2];
    // char* argv[MAXARGS];
    // char output[MAXARGS];
    // char buffer[MAXARGS];

    // memset(argv, 0, MAXARGS);
    // argv[0] = "/bin/bash";
    // argv[1] = "-c";
    // argv[2] = "cat prova.c | grep NULL";
    // argv[3] = NULL;

    // if (pipe(pipefd) == -1)
    //     perror("pipe() failed");

    // /* Fork() + exec() to execute the command */
    // pid_t pid = fork();

    // if (pid == -1)
    //     perror("fork() failed");
    // else if (pid > 0) {
    //     /* Inside parent */
    //     close(pipefd[1]);
    //     int nbytes = 0;
    //     memset(output, 0, MAXARGS);
    //     memset(buffer, 0, MAXARGS);
    //     output[0] = '\0';
    //     buffer[0] = '\0';

    //     while((nbytes = read(pipefd[0], buffer, sizeof(buffer))) != 0) {
    //         strcat(output, buffer);
    //         memset(buffer, 0, MAXARGS);
    //     }

    //     printf("Output: %s\n", output);
    //     int status;
    //     waitpid(pid, &status, 0);
    //     printf("Exit status: %d\n", status);

    // }
    // else {
    //     /* Inside child */
    //     dup2 (pipefd[1], STDOUT_FILENO);
    //     dup2 (pipefd[1], STDERR_FILENO);
    //     close(pipefd[0]);
    //     close(pipefd[1]);
    //     if (execve(argv[0], argv, NULL) == -1)  /* execve returns only in case of error */
    //         perror("Could not execve()");
    // }

    char cmd[MAXARGS] = "run \"paolo | grep > franco.txt\"";
    char* token = strtok(cmd, " \t\n");
    printf("TOKEN: %s\n", token);
    while ((token = strtok(NULL, " \t\n")) != NULL) {
        printf("TOKEN: %s\n", token);
    }

    return 0;
}