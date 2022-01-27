// Assignment #3: smallsh
// Filename: smallsh.h
// Author: Paul Scrugham
// ONID #: 934308765
// Date Due: 2/7/2022

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct userInput
{
    char* cmd;
    char* args[513];
};

struct userInput* parseInput(char* inputString) {
    struct userInput* currInput = malloc(sizeof(struct userInput));
    
    // For use with strtok_r
    char* saveptr;

    // Tokenize the user input and store in an array
    char* token = strtok_r(inputString, " ", &saveptr);
    currInput->args[0] = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currInput->args[0], token);

    int i = 1;
    while (*saveptr) {
        token = strtok_r(NULL, " ", &saveptr);
        currInput->args[i] = calloc(strlen(token) + 1, sizeof(char));
        strcpy(currInput->args[i], token);
        i++;
    }

    // Null terminate the arguments array
    currInput->args[i] = NULL;

    // Set the command variable
    currInput->cmd = calloc(strlen(currInput->args[0]) + 1, sizeof(char));
    strcpy(currInput->cmd, currInput->args[0]);

    return currInput;
}