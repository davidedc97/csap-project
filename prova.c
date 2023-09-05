#include <stdio.h>          /* for printf() and fprintf() */
#include <stdlib.h>         /* for atoi() and exit() */
#include <string.h>         /* for memset() */
#include <unistd.h>         /* for close() and getuid() */
#include <sys/wait.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>

#define MAXARGS 512

// int main(){
//     // int pipefd[2];
//     // char* argv[MAXARGS];
//     // char output[MAXARGS];
//     // char buffer[MAXARGS];

//     // memset(argv, 0, MAXARGS);
//     // argv[0] = "/bin/bash";
//     // argv[1] = "-c";
//     // argv[2] = "cat prova.c | grep NULL";
//     // argv[3] = NULL;

//     // if (pipe(pipefd) == -1)
//     //     perror("pipe() failed");

//     // /* Fork() + exec() to execute the command */
//     // pid_t pid = fork();

//     // if (pid == -1)
//     //     perror("fork() failed");
//     // else if (pid > 0) {
//     //     /* Inside parent */
//     //     close(pipefd[1]);
//     //     int nbytes = 0;
//     //     memset(output, 0, MAXARGS);
//     //     memset(buffer, 0, MAXARGS);
//     //     output[0] = '\0';
//     //     buffer[0] = '\0';

//     //     while((nbytes = read(pipefd[0], buffer, sizeof(buffer))) != 0) {
//     //         strcat(output, buffer);
//     //         memset(buffer, 0, MAXARGS);
//     //     }

//     //     printf("Output: %s\n", output);
//     //     int status;
//     //     waitpid(pid, &status, 0);
//     //     printf("Exit status: %d\n", status);

//     // }
//     // else {
//     //     /* Inside child */
//     //     dup2 (pipefd[1], STDOUT_FILENO);
//     //     dup2 (pipefd[1], STDERR_FILENO);
//     //     close(pipefd[0]);
//     //     close(pipefd[1]);
//     //     if (execve(argv[0], argv, NULL) == -1)  /* execve returns only in case of error */
//     //         perror("Could not execve()");
//     // }

//     char cmd[MAXARGS] = "run \"paolo | grep > franco.txt\"";
//     char* token = strtok(cmd, " \t\n");
//     printf("TOKEN: %s\n", token);
//     while ((token = strtok(NULL, " \t\n")) != NULL) {
//         printf("TOKEN: %s\n", token);
//     }

//     return 0;
// }



const char *semName = "asdfsd";

void parent(void){
    sem_t *sem_id = sem_open(semName, O_CREAT, 0600, 0);

    if (sem_id == SEM_FAILED){
        perror("Parent  : [sem_open] Failed\n"); return;
    }

    printf("Parent  : Wait for Child to Print\n");
    if (sem_wait(sem_id) < 0)
        printf("Parent  : [sem_wait] Failed\n");
    printf("Parent  : Child Printed! \n");
    
    if (sem_close(sem_id) != 0){
        perror("Parent  : [sem_close] Failed\n"); return;
    }

    if (sem_unlink(semName) < 0){
        printf("Parent  : [sem_unlink] Failed\n"); return;
    }
}

void child(void)
{
    sem_t *sem_id = sem_open(semName, O_CREAT, 0600, 0);

    if (sem_id == SEM_FAILED){
        perror("Child   : [sem_open] Failed\n"); return;        
    }

    printf("Child   : I am done! Release Semaphore\n");
    if (sem_post(sem_id) < 0)
        printf("Child   : [sem_post] Failed \n");
}

int main(int argc, char *argv[])
{
    pid_t pid;
    pid = fork();

    if (pid < 0){
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (!pid){
        child();
        printf("Child   : Done with sem_open \n");
    }
    else{
        int status;
        parent();
        wait(&status);
        printf("Parent  : Done with sem_open \n");
    }

    return 0;
}