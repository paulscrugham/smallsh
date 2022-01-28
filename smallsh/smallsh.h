// Assignment #3: smallsh
// Filename: smallsh.h
// Author: Paul Scrugham
// ONID #: 934308765
// Date Due: 2/7/2022

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <sys/types.h>

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


int countStringInstances(char* var, char* sentence) {
    int numInstances = 0;
    char* start = sentence;

    while ((start = strstr(start, var)) != NULL) {
        numInstances++;
        start = start + strlen(var);
    }

    return numInstances;
}


int expandVar(char* var, char* newVar, char* sentence) {
    int sentinel = 0;
    char* start = sentence;
    char* next;

    // Resize sentence to fit new expanded variables
    int numVars = countStringInstances(var, sentence);
    int delta = (strlen(newVar) - strlen(var)) * numVars;
    sentence = realloc(sentence, (strlen(sentence) + delta) * sizeof(char));

    // Check if var exists in sentence
    while ((start = strstr(start, var)) != NULL) {
        // Replace this occurrence of var in sentence with newVar
        
        next = strdup(start + strlen(var));
        *start = 0;
        strcat(sentence, newVar);
        strcat(sentence, next);
        start = start + strlen(newVar);

        sentinel = 1;
    }
    return sentinel;
}

int isComment(char* inputString, char commentChar) {
    // Extract first space delimited token
    char* token = strtok(inputString, " ");

    if (token[0] == commentChar) {
        return 1;
    }
    else {
        return 0;
    }
}