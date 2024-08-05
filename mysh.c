/**
 * This is the main file of myshell. It handles the primary control flow, flags, macros, and more.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h> 

#include "mysh.h"
#include "readline.h"
#include "commandlist.h"
#include "commandhandler.h"
#include "utils.h"

CommandList *parse_line(char *);

// Flags
bool INTERACTIVE_MODE = false;
bool RUNNING = true;

int main(int argc, char** argv) {
    // Decide what input to read from, and whether to run in interactive mode
    int fd;
    if (argc == 1) {
        fd = STDERR_FILENO;
        if (isatty(fd)) {
            INTERACTIVE_MODE = true;
        }
    } else if (argc == 2) {
        fd = open(argv[1], O_RDONLY);
        if (fd == -1) {
            perror("Error opening file");
            return EXIT_FAILURE;
        }
    } else {
        fprintf(stderr, "Too many arguments\n");
        return EXIT_FAILURE;
    }

    if (INTERACTIVE_MODE) {
        printf("Welcome to my shell!\n");
    }

    // Iterate through the commands until the file ends or until the running flag is disabled.
    while (RUNNING) {
        if (INTERACTIVE_MODE) {
            printf("mysh> ");
            fflush(stdout);
        }

        // Read the comments line by line
        char *line = readline(fd);
        if (line == NULL) {
            break;
        }

        execute_line(line);
    }

    // End with this prompt if in interactive mode
    if (INTERACTIVE_MODE) {
        printf("Exiting my shell.\n");
    }

    if (fd != STDIN_FILENO) {
        close(fd);
    }

    return EXIT_SUCCESS;
}

void execute_line(char *line) {
    CommandList *list = parse_line(line);
    Command *command = list->head;
    while (command != NULL) {
        execute_command(command);
        command = command->next;
    }
}

CommandList *parse_line(char *line) {
    char **tokens = parse_tokens(line);
    int index = 0;
    CommandList *list = create_list(tokens);
    Command *command = NULL;
    while (tokens[index] != NULL) {
        if (strcmp(tokens[index], ">") == NULL) {
            if (command != NULL && tokens[++index] != NULL) {
                command->output = tokens[index];
            } else {
                perror("Parsing error");
                exit(EXIT_FAILURE);
            }
        } else if (strstr(tokens[index], "|") != NULL) {
            if (command != NULL) {
                add_command(command);
                add_command(create_command(tokens[index]));
                command = NULL;
            } else {
                perror("Parsing error");
                exit(EXIT_FAILURE);
            }
        } else if (strstr(tokens[index], "<") != NULL) {
            if (command != NULL && tokens[++index] != NULL) {
                command->argc++;
                command->argv = realloc(command->argv, command->argc);
                if (command->argv == NULL) {
                    perror("Realloc fail");
                    exit(EXIT_FAILURE);
                }
                command->argv[command->argc - 1] = tokens[index];
            } else {
                perror("Parsing error");
                exit(EXIT_FAILURE);
            }
        } else {
            if (command == NULL) {
                command = create_command(tokens[index]);
            } else {
                command->argc++;
                command->argv = realloc(command->argv, command->argc);
                if (command->argv == NULL) {
                    perror("Realloc fail");
                    exit(EXIT_FAILURE);
                }
                command->argv[command->argc - 1] = tokens[index];
            }
        }   
        index++;     
    }

    if (command != NULL) {
        add_command(command);
    }

    return list;
}

void set_running(bool value) {
    RUNNING = value;
}