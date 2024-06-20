/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Yuvraj Gupta
* Student ID:: 922933190
* GitHub-Name:: YuvrajGupta1808
* Project:: Assignment 3 – Simple Shell with Pipes
*
* File:: Gupta_Yuvraj_HW3_main.c
*
* Description:: 
*
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE 163
#define TOKEN_SIZE 64
#define TOKEN_DELIMITERS " \t\r\n\a"

// Function prototypes
void shell_loop(char *prompt);
char *parse_input(char *input, const char *delimiter);
void execute_command(char *args[]);
void execute_piped_commands(char *commands[], int num_commands);

int main(int argc, char *argv[]) {
    // Set default prompt and update it if an argument is provided
    char *prompt = "> ";
    if (argc > 1) {
        prompt = argv[1];
    }
    shell_loop(prompt);
    return 0;
}

// Main loop to repeatedly read input, parse it, and execute commands
void shell_loop(char *prompt) {
    char input[BUFFER_SIZE];
    char *commands[TOKEN_SIZE];
    int line_size;

    while (1) {
        // Print the prompt
        printf("%s", prompt);

        // Read a line of input from the user
        if (fgets(input, BUFFER_SIZE, stdin) == NULL) {
            if (feof(stdin)) {
                // Handle EOF by exiting the loop
                printf("\n");
                break;
            } else {
                // Handle reading error and continue the loop
                perror("Reading error");
                continue;
            }
        }

        // Remove the newline character at the end of the input
        line_size = strlen(input);
        if (line_size > 0 && input[line_size - 1] == '\n') {
            input[line_size - 1] = '\0';
        }

        // If the input is empty, print a message and continue the loop
        if (input[0] == '\0') {
            printf("Empty input\n");
            continue;
        }

        // Split the input into commands separated by pipes
        int num_commands = 0;
        char *command = parse_input(input, "|");
        while (command != NULL) {
            commands[num_commands++] = command;
            command = parse_input(NULL, "|");
        }
        commands[num_commands] = NULL;

        // Execute commands: single command or piped commands
        if (num_commands == 1) {
            // No pipe: parse and execute a single command
            char *args[TOKEN_SIZE];
            int num_args = 0;
            char *arg = parse_input(commands[0], TOKEN_DELIMITERS);
            while (arg != NULL) {
                args[num_args++] = arg;
                arg = parse_input(NULL, TOKEN_DELIMITERS);
            }
            args[num_args] = NULL;
            execute_command(args);
        } else {
            // Multiple commands: execute piped commands
            execute_piped_commands(commands, num_commands);
        }
    }
}

// Parse input string into tokens based on the given delimiter
char *parse_input(char *input, const char *delimiter) {
    static char *saveptr;
    if (input != NULL) {
        return strtok_r(input, delimiter, &saveptr);
    } else {
        return strtok_r(NULL, delimiter, &saveptr);
    }
}

// Execute a single command using fork and execvp
void execute_command(char *args[]) {
    pid_t pid, wpid;
    int status;

    // Handle exit command
    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    }

    // Fork a child process to execute the command
    pid = fork();
    if (pid == 0) {
        // In the child process, execute the command
        if (execvp(args[0], args) == -1) {
            perror("shell");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Handle error in forking
        perror("shell");
    } else {
        // In the parent process, wait for the child process to complete
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        printf("Child %d exited with status %d\n", pid, WEXITSTATUS(status));
    }
}

// Execute multiple piped commands
void execute_piped_commands(char *commands[], int num_commands) {
    int i;
    int pipefd[2 * (num_commands - 1)];
    pid_t pid;
    int status;

    // Create all necessary pipes
    for (i = 0; i < num_commands - 1; i++) {
        if (pipe(pipefd + i * 2) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < num_commands; i++) {
        // Fork a child process for each command
        pid = fork();
        if (pid == 0) {
            // In the child process, set up input/output redirection
            if (i > 0) {
                // Redirect input from the previous pipe
                if (dup2(pipefd[(i - 1) * 2], 0) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }
            if (i < num_commands - 1) {
                // Redirect output to the next pipe
                if (dup2(pipefd[i * 2 + 1], 1) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            // Close all pipe file descriptors
            for (int j = 0; j < 2 * (num_commands - 1); j++) {
                close(pipefd[j]);
            }

            // Parse and execute the command
            char *args[TOKEN_SIZE];
            int num_args = 0;
            char *arg = parse_input(commands[i], TOKEN_DELIMITERS);
            while (arg != NULL) {
                args[num_args++] = arg;
                arg = parse_input(NULL, TOKEN_DELIMITERS);
            }
            args[num_args] = NULL;
            if (execvp(args[0], args) == -1) {
                perror("shell");
            }
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            // Handle error in forking
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    // Parent process closes all pipe file descriptors
    for (i = 0; i < 2 * (num_commands - 1); i++) {
        close(pipefd[i]);
    }

    // Wait for all child processes to complete and print their status
    for (i = 0; i < num_commands; i++) {
        pid_t wpid = wait(&status);
        if (wpid > 0) {
            printf("Child %d exited with status %d\n", wpid, WEXITSTATUS(status));
        }
    }
}
