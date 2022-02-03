// Assignment #3: smallsh
// Filename: smallsh.h
// Author: Paul Scrugham
// ONID #: 934308765
// Date Due: 2/7/2022

#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <sys/wait.h>
#include <sys/types.h>

struct userInput
{
    char* args[514];
    char* inputRedir;
    char* outputRedir;
    int background;
};

struct userInput* parseInput(char* inputString) {
    struct userInput* currInput = malloc(sizeof(struct userInput));

    // Initialize struct values
    currInput->inputRedir = NULL;
    currInput->outputRedir = NULL;
    currInput->background = 0;
    
    // For use with strtok_r
    char* saveptr;

    // Get the provided command
    char* token = strtok_r(inputString, " ", &saveptr);
    currInput->args[0] = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currInput->args[0], token);

    // Get command arguments
    int i = 1;
    while (*saveptr && *saveptr != '<' && *saveptr != '>' && *saveptr != '&') {
        token = strtok_r(NULL, " ", &saveptr);
        currInput->args[i] = calloc(strlen(token) + 1, sizeof(char));
        strcpy(currInput->args[i], token);
        i++;
    }

    // Get redirect statements, if any
    while (*saveptr && *saveptr != '&') {
        token = strtok_r(NULL, " ", &saveptr);
        if (*token == '<') {
            token = strtok_r(NULL, " ", &saveptr);
            currInput->inputRedir = calloc(strlen(token) + 1, sizeof(char));
            strcpy(currInput->inputRedir, token);
        }
        else if (*token == '>') {
            token = strtok_r(NULL, " ", &saveptr);
            currInput->outputRedir = calloc(strlen(token) + 1, sizeof(char));
            strcpy(currInput->outputRedir, token);
        }
    }

    // Check if last token is an &
    if (*saveptr && *saveptr == '&') {
        currInput->background = 1;
    }

    // Null terminate the arguments array
    currInput->args[i] = NULL;

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

int isEmpty(char* inputString) {
    // Extract first space delimited token
    char* temp = strdup(inputString);
    char* token = strtok(temp, " ");

    if (token) {
        free(temp);
        return 0;
    }
    else {
        free(temp);
        return 1;
    }
}

int isComment(char* inputString, char commentChar) {
    // Extract first space delimited token
    char* temp = strdup(inputString);
    char* token = strtok(temp, " ");

    if (token[0] == commentChar) {
        free(temp);
        return 1;
    }
    else {
        free(temp);
        return 0;
    }
}

void cdBuiltIn(char* dirPath) {
    if (dirPath) {
        chdir(dirPath);
    }
    else {
        chdir(getenv("HOME"));
    }
}

void runArbitrary(struct userInput* input) {
    // Fork a new process
    pid_t spawnPid = fork();
    int childStatus;

    switch (spawnPid) {
    case -1:
        perror("fork()\n");
        exit(1);
        break;
    case 0:
        // Check for input redirection
        if (input->inputRedir) {
            // Create file descriptor
            int fd = open(input->inputRedir, O_RDONLY);
            if (fd == -1) {
                // TODO: Set exit status
                printf("open() failed on \"%s\"\n", input->inputRedir);
                perror("Error");
                exit(1);
            }
            // TODO: close fd

            if (dup2(fd, 0) != -1) {
                printf("dup2 succeeded for input redirection!");
            }
        }

        // Check for input redirection
        if (input->outputRedir) {
            int fd = open(input->outputRedir, O_WRONLY | O_CREAT | O_TRUNC, 0640);
            if (fd == -1) {
                printf("open() failed on \"%s\"\n", input->inputRedir);
                perror("Error");
                exit(1);
            }
            // TODO: close fd

            if (dup2(fd, 1) != -1) {
                printf("dup2 succeeded for output redirection!");
            }
        }

        // Run an arbitrary command using execvp
        execvp(input->args[0], input->args);
        perror("execvpe");
        exit(2);
        break;
    default:
        if (input->background) {
            printf("background pid is %d\n", getpid());
            spawnPid = waitpid(spawnPid, &childStatus, WNOHANG);
            // TODO: print status when background job is complete
        }
        else {
            spawnPid = waitpid(spawnPid, &childStatus, 0);
        }
        // exit(0);
        break;
    }
}