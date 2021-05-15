/*
    * Author: Anuj Ramesh
    * EECS 3221 Project 1 - Simple Unix Shell
    * Feb 7, 2021
*/

#include <stdio.h>
#include <unistd.h>     
#include <sys/types.h> 
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 80
#define TOKEN_DELIM " "

char history[MAX_LINE]; // A global variable to store the previous command

int main(void) {
        
    int should_run = 1;
    int prevCommands = 0;   // A counter of how many commands have been executed

    while (should_run) {

        char *args[MAX_LINE/2 + 1];
        char * pipelineArgs[8]; 
        char command[MAX_LINE]; 
        char *token;
        char fileName[MAX_LINE];
        int hasAmper = 0;       // 1 if ampersand at the end, 0 otherwise
        int inputRedir = 0;     // Argument position of "<" if command contains it, 0 otherwise
        int outputRedir = 0;    // Argument position of ">" if command contains it, 0 otherwise
        int hasPipe = 0;
        int i, j;
        int fd[2];

        pid_t pid;

        printf("mysh:~$ ");
        fflush(stdout);

        // Get user input 
        fgets(command, MAX_LINE, stdin);
        command[strlen(command) - 1] = '\0'; // Remove the \n from fgets, make it null

        if (strcmp(command, "!!") != 0) {   // command is not "!!"

            strcpy(history, command);   // global variable history will contain the current command

            // tokenize the user input
            token = strtok(command, TOKEN_DELIM);
  
        } else {

            if (prevCommands == 0) {      // If there hasn't been a valid command yet, restart loop till a valid command is entered
                printf("No commands in history.\n");
                continue;
            }

            strcpy(command, history);   // The current command will now be the previous command
            printf("%s\n", history);
            token = strtok(command, TOKEN_DELIM);
        }

            i = 0;
            j = 0;
            while (token != NULL) {

                if (strcmp(token, "<") == 0) {
                    inputRedir = i;
                    token = strtok(NULL, TOKEN_DELIM);
                    strcpy(fileName, token);
                    break;
                }
                else if (strcmp(token, ">") == 0) {
                    outputRedir = i;
                    token = strtok(NULL, TOKEN_DELIM);
                    strcpy(fileName, token);
                    break;
                }
                else if (strcmp(token, "|") == 0) {
                    hasPipe = i;
                    token = strtok(NULL, TOKEN_DELIM);
                    while (token != NULL) {
                        pipelineArgs[j] = token;
                        j++;
                        token = strtok(NULL, TOKEN_DELIM);
                    }
                    pipelineArgs[j] = NULL;
                    break;
                }
                else {
                    args[i] = token;
                }

                i++;
                token = strtok(NULL, TOKEN_DELIM);
            }

            if (strcmp(args[i-1], "&") == 0) {
                hasAmper = 1;
                args[i-1] = NULL;
            } else {
                args[i] = NULL;
            }

        if (hasPipe) {
            if (pipe(fd) == -1) {
                return 1;
            }
        }

        // Fork a child process
        pid = fork();
        int status;

        if (pid < 0) {  // error occurred
            fprintf(stderr, "Fork Failed");
            return 1;
        } 
        else if (pid == 0) {    // child process

            if (outputRedir) {  // The arg 1 index to the right of ">" is the file name
                int fdOutput = open(fileName, O_WRONLY | O_CREAT, 0664);

                if (fdOutput == -1) {
                    return 2;
                }

                dup2(fdOutput, STDOUT_FILENO);
                close(fdOutput);

            }
            else if (inputRedir) {
                int fdInput = open(fileName, O_RDONLY);

                if (fdInput == -1) {
                    return 2;
                }

                dup2(fdInput, STDIN_FILENO);
                close(fdInput);
            }

            if (hasPipe) {
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
            }

            if (hasAmper) {
                int devNull = open("/dev/null", O_WRONLY);
                
                if (devNull == -1) {
                    return 2;
                }
                
                dup2(devNull, STDOUT_FILENO);
                close(devNull);
                setpgrp();
            }

            execvp(args[0], args);
            break;
        }
        else {  // parent process
            if (!hasAmper) {    // If there is no ampersand, wait for child. Otherwise, don't wait for child
                waitpid(pid, NULL, 0);
            }

            if (hasPipe) {
                pid_t pid2 = fork();

                if (pid2 < 0) {
                    return 3;
                }

                if (pid2 == 0) {
                    dup2(fd[0], STDIN_FILENO);
                    close(fd[0]);
                    close(fd[1]);
                    execvp(pipelineArgs[0], pipelineArgs);
                }

                close(fd[0]);
                close(fd[1]);

                waitpid(pid, NULL, 0);
                waitpid(pid2, NULL, 0);
            }

            if (strcmp(command, "cd") == 0) {
                chdir(args[1]);
            } 
            else if (strcmp(command, "exit") == 0) {
                break;
            }
        }

        prevCommands++;
    }

    return 0;
}