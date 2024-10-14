#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include "cmdparse.h"

int main() {
    while (1) {
        printf("$ ");
        char input[64];
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break; // Exit loop on EOF
        }

        if (strlen(input) == 1) {
            continue; // Skip empty command
        }

        CMD cmd;

        int parsedInput = cmdparse(input, &cmd);

        if (parsedInput == 0) {
            pid_t pid = fork();
            if (pid == 0) {
                if (cmd.redirectIn) {
                    int inFile = open(cmd.infile, O_RDONLY);
                    if (inFile == -1) {
                        perror("open");
                        exit(1);
                    }
                    dup2(inFile, STDIN_FILENO);
                    close(inFile);
                }
                if (cmd.redirectOut) {
                    int outFile;
                    if (cmd.redirectAppend) {
                        outFile = open(cmd.outfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    }
                    else {
                        outFile = open(cmd.outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    }

                    if (outFile == -1) {
                        perror("open");
                        exit(1);
                    }
                    dup2(outFile, STDOUT_FILENO);
                    close(outFile);
                }
                if (!cmd.pipelining) {
                    execvp(cmd.argv1[0], cmd.argv1);
                    perror("execvp");
                    exit(1);
                }
                else {
                    int pipefd[2];
                    if (pipe(pipefd) == -1) {
                        perror("pipe");
                        exit(1);
                    }
                    pid_t child_pid = fork();
                    if (child_pid == 0) {
                        close(pipefd[0]);
                        dup2(pipefd[1], STDOUT_FILENO);
                        close(pipefd[1]);
                        execvp(cmd.argv1[0], cmd.argv1);
                        perror("execvp");
                        exit(1);
                    }
                    else if (child_pid > 0) {
                        int status;
                        waitpid(child_pid, &status, 0);
                        close(pipefd[1]);
                        dup2(pipefd[0], STDIN_FILENO);
                        close(pipefd[0]);

                        execvp(cmd.argv2[0], cmd.argv2);
                        perror("execvp");
                        exit(1);
                    }
                    else {
                        perror("fork");
                        exit(1);
                    }
                }
            }
            else if (pid > 0) {
                if (!cmd.background) {
                    int status;
                    waitpid(pid, &status, 0);
                }
            }
            else {
                perror("fork");
                exit(1);
            }
        }
        else {
            perror("cmdparse");
        }
    }

    return 0;
}
