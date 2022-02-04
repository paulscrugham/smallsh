// Assignment #3: smallsh
// Filename: smallsh.c
// Author: Paul Scrugham
// ONID #: 934308765
// Date Due: 2/7/2022

#include "smallsh.h"

int main(void)
{
	char* prompt = ": ";
	char* exVar = "$$";
	int sentinel = 1;
	int status = 0;
	int lastBackgroundPid;
	char* exitStatusMessage = "exit value";
	char* termStatusMessage = "terminated by signal";

	while (sentinel) {
		char* inputString = NULL;
		size_t buflen = 0;
		struct userInput* input;

		// TODO: Check status of any open background child processes and
		// - print exit status if ended
		// - clean up if ended

		// Prompt user for input
		printf(prompt);
		getline(&inputString, &buflen, stdin);

		// Strip newline character from inputString
		if (strlen(inputString) > 1) {
			inputString[strcspn(inputString, "\n")] = 0;
		}

		// Check if input is empty
		if (isEmpty(inputString)) {
			continue;
		}
		
		// Check if input was a comment
		if (isComment(inputString, '#')) {
			continue;
		}

		// Expand any occurrences of "$$" to the program's PID
		pid_t pid = getpid();
		char* newVar = malloc(sizeof(char) * 21);
		sprintf(newVar, "%d", pid);
		expandVar(exVar, newVar, inputString);
		free(newVar);

		// Parse input
		input = parseInput(inputString);

		// Check if command is a smallsh built-in
		if (strcmp(input->args[0], "exit") == 0) {
			// TODO: kill any other processes or jobs that smallsh started
			return 0;
		}
		else if (strcmp(input->args[0], "cd") == 0) {
			// Run command
			cdBuiltIn(input->args[1]);
		}
		else if (strcmp(input->args[0], "status") == 0) {
			if (WIFEXITED(status)) {
				printf("%s %d\n", exitStatusMessage, WEXITSTATUS(status));
			}
			else if (WIFSIGNALED(status)) {
				printf("%s %d\n", termStatusMessage, WTERMSIG(status));
			}
		}
		// Run an arbitrary command
		else {
			
			if (input->background) {
				// If background process, runArbitrary returns the child pid
				lastBackgroundPid = runArbitrary(input);
				printf("background child pid in main = %d\n", lastBackgroundPid);
			}
			else {
				// If foreground process, runArbitrary returns a status code
				status = runArbitrary(input);
				printf("foreground child pid in main = %d\n", status);
			}
			
		}

		// Print output
		free(inputString);
	}
	return 0;
}