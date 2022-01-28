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

	while (sentinel) {
		char* inputString = NULL;
		size_t buflen = 0;
		struct userInput* input;
		// Prompt user for input
		printf(prompt);
		getline(&inputString, &buflen, stdin);

		// Strip newline character from inputString
		if (strlen(inputString) > 1) {
			inputString[strcspn(inputString, "\n")] = 0;
		}
		
		// Check if input was a comment
		if (isComment(inputString, '#')) {
			continue;
		}

		// Expand any occurrences of "$$" to the program's PID
		pid_t pid = getpid();
		char* newVar = malloc(sizeof(char) * 21);
		sprintf(newVar, "%d", pid);
		if (expandVar(exVar, newVar, inputString)) {
			printf("Variables expanded successfully!\n");
			printf(inputString);
			printf("\n");
		}
		free(newVar);

		// Parse input
		input = parseInput(inputString);

		if (strcmp(input->cmd, "exit") == 0) {
			// TODO: kill any other processes or jobs that smallsh started
			return 0;
		}
		else if (strcmp(input->cmd, "cd") == 0) {
			// Run command
			cdBuiltIn(input->args[1]);

			// test print
			char* workingDir;
			workingDir = malloc(256);
			size_t wdBufLen = 256;
			getcwd(workingDir, wdBufLen);
			printf("TEST PRINT - CWD: ");
			printf(workingDir);
			printf("\n");
		}
		else if (strcmp(input->cmd, "status") == 0) {
			// Run command

		}

		// Print output
		free(inputString);
	}
	return 0;
}