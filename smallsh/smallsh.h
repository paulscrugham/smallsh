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
#include <errno.h>
#include <signal.h>

volatile sig_atomic_t flag = 0;

void sigtstpOff(int);

/* Handler function for SIGTSTP to turn on background only mode */
void sigtstpOn(int signo) {
    flag = 1;
    const char message[] = "\nEntering foreground-only mode (& is now ignored)\n: ";
    write(STDOUT_FILENO, message, sizeof message - 1);
    // Set signal action to function that turns SIGTSTP off for next signal
    signal(SIGTSTP, &sigtstpOff);
}

/* Handler function for SIGTSTP to turn off background only mode */
void sigtstpOff(int signo) {
    flag = 0;
    const char message[] = "\nExiting foreground-only mode\n: ";
    write(STDOUT_FILENO, message, sizeof message - 1);
    // Set signal action to function that turns SIGTSTP on for next signal
    signal(SIGTSTP, &sigtstpOn);
}

/* Struct for tracking running, background child PIDs */
struct bgChildPIDs {
    pid_t pid;
    struct bgChildPIDs* next;
};

/* Creates a new linked list element to store a child pid. Returns the new element. */
struct bgChildPIDs* createChildPID(pid_t pid) {
    struct bgChildPIDs* currChildPID = malloc(sizeof(struct bgChildPIDs));
    currChildPID->pid = pid;
    currChildPID->next = NULL;

    return currChildPID;
}

/* 
Adds an element with the specified pid to the front of the specified list.
Returns the new linked list head.
*/
struct bgChildPIDs* addChildPID(pid_t pid, struct bgChildPIDs* list) {
    struct bgChildPIDs* newChild = createChildPID(pid);
    newChild->next = list;
    return newChild;
}

/*
Removes the matching element from the specified list.
Returns a pointer to the head of the list.
*/
struct bgChildPIDs* removeChildPID(pid_t pid, struct bgChildPIDs* list) {
    // Case if first item matches
    if (list->pid == pid) {
        return list->next;
    }
    // Case if item exists in the remainder of the list
    else {
        struct bgChildPIDs* prev = list;
        struct bgChildPIDs* curr = list->next;
        while (curr) {
            if (curr->pid == pid) {
                // Remove current element from linked list
                prev->next = curr->next;
                free(curr);
                return list;
            }
            prev = curr;
            curr = curr->next;
        }
    }
    // Return list head if element is not found
    return list;
}

/* Uses waitpid with WNOHANG to clean up any zombie child processes and print their exit status. */
void waitBG(int childPid, int bgStatus, struct bgChildPIDs* bgChildList, char* exit, char* term) {
    // Catch any background processes and print exit/termination status
    while ((childPid = waitpid(-1, &bgStatus, WNOHANG)) > 0) {
        printf("background pid %d is done: ", childPid);
        fflush(stdout);

        // Remove from bgChildList
        bgChildList = removeChildPID(childPid, bgChildList);

        // Update status
        if (WIFEXITED(bgStatus)) {
            printf("%s %d\n", exit, WEXITSTATUS(bgStatus));
            fflush(stdout);
        }
        else if (WIFSIGNALED(bgStatus)) {
            printf("%s %d\n", term, WTERMSIG(bgStatus));
            fflush(stdout);
        }
    }
}

/* Struct to store the parsed input string */
struct userInput {
    char* args[514];
    char* inputRedir;
    char* outputRedir;
    int background;
};

/* 
Parses the specified string for the command, arguments, I/O redirection, and the
background process flag. Returns a struct containing the parsed information.
*/
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
    while (*saveptr) {
        token = strtok_r(NULL, " ", &saveptr);
        // Check for input redirection
        if (strcmp(token, "<") == 0) {
            token = strtok_r(NULL, " ", &saveptr);
            currInput->inputRedir = calloc(strlen(token) + 1, sizeof(char));
            strcpy(currInput->inputRedir, token);
        }
        // Check for output redirection
        else if (strcmp(token, ">") == 0) {
            token = strtok_r(NULL, " ", &saveptr);
            currInput->outputRedir = calloc(strlen(token) + 1, sizeof(char));
            strcpy(currInput->outputRedir, token);
        }
        // Check for background flag
        else if ((strcmp(token, "&") == 0) && !*saveptr) {
            currInput->background = 1;
        }
        // Get command args
        else {
            currInput->args[i] = calloc(strlen(token) + 1, sizeof(char));
            strcpy(currInput->args[i], token);
            i++;
        }
    }

    // Null terminate the arguments array
    currInput->args[i] = NULL;

    return currInput;
}

/* Checks if the specified string consists of only space characters. Returns 1 if empty, and 0 if not. */
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

/* Checks if the specified string begins with the # character. Returns 1 if it does, and 0 if not. */
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

/* 
Checks if the specified string is a valid input (i.e. is not empty and is not a comment).
Returns 1 if valid, and 0 if not. 
*/
int preParser(char* inputString) {
    if (strlen(inputString) == 1) {
        return 0;
    }

    // Check if input is empty
    if (isEmpty(inputString)) {
        return 0;
    }

    // Check if input was a comment
    if (isComment(inputString, '#')) {
        return 0;
    }
    
    return 1;
}

/* Counts the number of instances of var in sentence. Returns the count as an integer. */
int countStringInstances(char* var, char* sentence) {
    int numInstances = 0;
    char* start = sentence;

    while ((start = strstr(start, var)) != NULL) {
        numInstances++;
        start = start + strlen(var);
    }

    return numInstances;
}

/* 
Replaces as many instances of oldVar in inputString as specified by the integer numVars.
Returns a new string with the expanded variables. Does not modify inputString. 
*/
char* expandVar(char* inputString, int numVars, char* oldVar) {
    // Get PID as string
    pid_t pid = getpid();
    char* newVar = calloc(21, sizeof(char));
    sprintf(newVar, "%d", pid);

    // Calculate string length delta
    int delta = (strlen(newVar) - strlen(oldVar)) * numVars;
    // Duplicate and allocate memory for new string
    char* result = strdup(inputString);
    result = realloc(result, ((strlen(inputString) + delta) + 1) * sizeof(char));

    char* start = result;
    char* next;
    // Construct new string
    while ((start = strstr(start, oldVar)) != NULL) {
        // Replace this occurrence of var in sentence with newVar
        next = strdup(start + strlen(oldVar));
        *start = 0;  // Trim the "result" string up to the variable to be replaced
        strcat(result, newVar);
        strcat(result, next);
        start = start + strlen(newVar);
        free(next);
    }
    free(newVar);

    return result;
}

/* Built in shell command that prints the status of the last foreground process to run. */
void statusBuiltIn(int fgStatus, char* exit, char* term) {
    if (fgStatus == -1) {
        printf("%s %d\n", exit, 0);
        fflush(stdout);
    }
    else if (WIFEXITED(fgStatus)) {
        printf("%s %d\n", exit, WEXITSTATUS(fgStatus));
        fflush(stdout);
    }
    else if (WIFSIGNALED(fgStatus)) {
        printf("%s %d\n", term, WTERMSIG(fgStatus));
        fflush(stdout);
    }
}

/* 
Built in shell command that exits the shell program. Kills any running background processes and
cleans up any zombie processes.
*/
int exitBuiltIn(struct bgChildPIDs* bgChildList, int bgStatus) {
    struct bgChildPIDs* curr = bgChildList;
    while (curr) {
        // Terminate process
        kill(curr->pid, SIGTERM);
        // Clean up dead process
        waitpid(curr->pid, &bgStatus, WNOHANG);
        curr = curr->next;
    }
    return 0;
}

/* 
Built in shell command that sets the current directory to the specified path, if provided.
If no path is provided, the current directory is set to HOME. 
*/
void cdBuiltIn(char* dirPath) {
    if (dirPath) {
        // Set current directory to specified path
        chdir(dirPath);
    }
    else {
        // Set current directory to HOME
        chdir(getenv("HOME"));
    }
}

/*  */
int redirectInput(char* newInput) {
    int fd = open(newInput, O_RDONLY);
    if (fd == -1) {
        printf("cannot open %s for input\n", newInput);
        fflush(stdout);
        exit(1);
    }
    else {
        dup2(fd, STDIN_FILENO);
    }

    return fd;
}

int redirectOutput(char* newOutput) {
    int fd = open(newOutput, O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if (fd == -1) {
        printf("cannot open %s for output\n", newOutput);
        fflush(stdout);
        exit(1);
    }
    else {
        dup2(fd, STDOUT_FILENO);
    }

    return fd;
}

int runArbitrary(struct userInput* input) {
    // Fork a new process
    pid_t spawnPid = fork();

    switch (spawnPid) {
    case -1:
        perror("fork()\n");
        fflush(stdout);
        exit(1);
    case 0:
        // Check for input redirection
        if (input->inputRedir) {
            redirectInput(input->inputRedir);
        }
        else if (input->background) {
            redirectInput("/dev/null");
        }

        // Check for output redirection
        if (input->outputRedir) {
            redirectOutput(input->outputRedir);
        }
        else if (input->background) {
            redirectOutput("/dev/null");
        }

        // If foreground process, reset SIGINT to default action
        if (!input->background) {
            struct sigaction SIGINT_action = { {0} };

            // Register handle_SIGINT as the signal handler
            SIGINT_action.sa_handler = SIG_DFL;
            // Block all catchable signals while handle_SIGINT is running
            sigemptyset(&SIGINT_action.sa_mask);
            // Restart flag for getline
            SIGINT_action.sa_flags = SA_RESTART;

            // Install the signal handler
            sigaction(SIGINT, &SIGINT_action, NULL);
        }

        // Set child ps to ignore SIGTSTP
        signal(SIGTSTP, SIG_IGN);

        // Run an arbitrary command using execvp
        execvp(input->args[0], input->args);
        perror(input->args[0]);
        exit(1);
        break;
    default:
        return spawnPid;
    }
}

void freeInputStruct(struct userInput* input) {
    // Clear args
    int i = 0;
    while (input->args[i] != 0) {
        free(input->args[i]);
        i++;
    }

    // Clear input redirection file name
    free(input->inputRedir);
    // Clear output redirection file name
    free(input->outputRedir);
    // Clear struct
    free(input);
}

void freeChildPIDList(struct bgChildPIDs* list) {
    struct bgChildPIDs* temp;

    while (list != NULL) {
        temp = list->next;
        free(list);
        list = temp;
    }
}