/**
 * This file manages a doubly linked list of commands, which allows for easier management of state.
*/

#include <stdio.h>
#include <stdlib.h>

#include "commandlist.h"

static CommandList *command_list = NULL;

Command *create_command(char *c) {
    Command *command = malloc(sizeof(Command));
    if (command == NULL) {
        return NULL;
    }
    command->argc = 1;
    command->argv = malloc(sizeof(char));
    command->argv[0] = c; 
    command->next = NULL;
    command->prev = NULL;
    command->status = -1;
    command->output = NULL;
    return command;
}

CommandList *create_list(char **tokens) {
    if (command_list == NULL) {
        command_list = malloc(sizeof(CommandList));
        command_list->head = NULL;
        command_list->tokens = tokens;
    }
    return command_list;
}

void print_list() {
    Command *temp = command_list->head;
    while (temp != NULL) {
        for (int i = 0; i < temp->argc; i++) {
            printf("%s, ", temp->argv[i]);
        }
        printf(" -> ");
        temp = temp->next;
    }
    printf("\n");
}

void add_command(Command *new_command) {
    if (command_list->head == NULL) {
        command_list->head = new_command;
    } else {
        Command *temp = command_list->head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = new_command;
        new_command->prev = temp;
    }
}

void remove_list() {
    if (command_list != NULL) {
        Command *current = command_list->head;
        Command *next = current;
        while (current != NULL) {
            next = current->next;
            free(current);
            current = next;
        }
        free(command_list->tokens);
        free(command_list);
    }    
}