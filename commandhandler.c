/**
 * This file handles each command provided by mysh and commandlist. It provides custom implementation of certain methods, as well
 * as a generic implementation provided by the Unix OS.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h> 

#include "mysh.h"
#include "commandlist.h"
#include "commandhandler.h"


void execute_command(Command *command) {
    char *cmd = command->argv[0];
    if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "cd") == 0 || strcmp(cmd, "which") == 0 || strcmp(cmd, "pwd") == 0) {
        handle_custom(command);
    } else if (strcmp(cmd, "|")) {

    } else {
        handle_default(command);
    }
}

handle_pipe(Command *command) {

}



void handle_custom(Command *command) {

}

void handle_default(Command *command) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        if (strchr(command->argv[0], '/') != NULL) {
            command->argv = realloc(command->argv, command->argc + 1);
            command->argv[command->argc] = NULL;
            execv(command->argv[0], command->argv);
            fprintf(stderr, "Command or executable not recognized.\n");
        } else {
            char *paths[] = {"/usr/local/bin", "/usr/bin", "/bin", NULL};
            for (int i = 0; paths[i] != NULL; i++) {
                char path[1024];
                snprintf(path, sizeof(path), "%s%s", paths[i], command->args[0]);
                if (access(path, X_OK) == 0) {
                    execv(path, command->argv);
                    perror("execv");
                    exit(EXIT_FAILURE);
                }
            }
            fprintf(stderr, "Command not found: %s\n", command->argv[0]);
        }
    } else {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
        // Handle any errors in child process execution
        if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
            fprintf(stderr, "Error executing command\n");
            exit(EXIT_FAILURE);
        }
    }
}
    

// void execute_command(Command *command) {
//     if (strcmp(command->cmd, "exit") == 0) {
//         if (command->argc > 0) {
//             for (int i = 0; i < command->argc; i++) {
//                 printf("%s ", command->argv[i]);
//             }
//             printf("\n");
//         }
//         set_running(false);

//     } else if (strcmp(command->cmd, "cd") == 0) {
//         if (command->argc == 1) {
//             if (chdir(command->argv[0]) != 0) {
//                 fprintf(stderr, "cd: No such file or directory");
//                 command->status = 0;
//             }
//             command->status = 1;
//         } else {
//             fprintf(stderr, "cd: Incorrect number of arguments\n");
//             command->status = 0;
//         }
//     } else if (strcmp(command->cmd, "pwd") == 0) {
//         char result[100];
//         if ((getcwd(result, 100)) != NULL) {
//             printf("%s\n", result);
//             command->status = 1;
//         } else {
//             fprintf(stderr, "pwd: Read or search permission was denied for a component of the pathname.\n");
//             command->status = 0;
//         }
//     } else if (strcmp(command->cmd, "which") == 0) {
//         if (command->argc == 1) {
//             command->argv[0];
//             if (strcmp(command->cmd, "pwd") == 0 || strcmp(command->cmd, "which") == 0 || strcmp(command->cmd, "cd") == 0 || strcmp(command->cmd, "exit") == 0) {
//                 fprintf(stderr, "which: Incorrect number of arguments\n");
//                 command->status = 0;
//             } else if (strchr(command->argc, "/") != NULL) {
//                 printf("%s\n",command->argc);
//             } else {
//                 char* dirs[] = {"/usr/local/bin/", "/usr/bin/", "/bin/"};
//                 int i;
//                 for (i = 0; i < 3; i++) {
//                     char fullpath[1024];
//                     snprintf(fullpath, sizeof(fullpath), "%s%s", dirs[i], name);
//                     if (access(fullpath, X_OK) == 0) {
//                         printf("%s\n", fullpath);
//                         break;
//                     }
//                 }
//             }

//             command->status = 1;
//         } else {
//             fprintf(stderr, "which: Incorrect number of arguments\n");
//             command->status = 0;
//         }
//     } else if (strcmp(command->cmd, "|") == 0) {
        
//     } else {
        
//     }
// }