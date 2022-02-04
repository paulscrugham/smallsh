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
	pid_t childPid;
	int fgStatus = -1;
	int bgStatus;
	char* exitStatusMessage = "exit value";
	char* termStatusMessage = "terminated by signal";

	while (sentinel) {
		char* inputString = NULL;
		size_t buflen = 0;
		
		while ((childPid = waitpid(-1, &bgStatus, WNOHANG)) > 0) {
			printf("background pid %d is done: ", childPid);
			if (WIFEXITED(bgStatus)) {
				printf("%s %d\n", exitStatusMessage, WEXITSTATUS(bgStatus));
				//fflush(stdout);
			}
			else if (WIFSIGNALED(bgStatus)) {
				printf("%s %d\n", termStatusMessage, WTERMSIG(bgStatus));
				//fflush(stdout);
			}
		}

		struct userInput* input;

		// Prompt user for input
		printf(prompt);
		getline(&inputString, &buflen, stdin);

		// Check for empty input string. If not empty, strip trailing newline char
		if (strlen(inputString) == 1) {
			continue;
		}
		else if (strlen(inputString) > 1) {
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
			if (fgStatus == -1) {
				printf("%s %d\n", exitStatusMessage, 0);
			}
			else if (WIFEXITED(fgStatus)) {
				printf("%s %d\n", exitStatusMessage, WEXITSTATUS(fgStatus));
				//fflush(stdout);
			}
			else if (WIFSIGNALED(fgStatus)) {
				printf("%s %d\n", termStatusMessage, WTERMSIG(fgStatus));
				//fflush(stdout);
			}
		}
		// Run an arbitrary command
		else {
			childPid = runArbitrary(input);
			// Handle foreground execution
			if (!input->background) {
				waitpid(childPid, &fgStatus, 0);
			}
			else {
				printf("background pid is %d\n", childPid);
				//fflush(stdout);
			}
		}

		// Print output
		free(inputString);
	}
	return 0;
}