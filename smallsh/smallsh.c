// Assignment #3: smallsh
// Filename: smallsh.c
// Author: Paul Scrugham
// ONID #: 934308765
// Date Due: 2/7/2022

#include "smallsh.h"

int main()
{
	char* inputString = NULL;
	size_t buflen = 0;
	struct userInput* input;
	int sentinel = 1;

	while (sentinel) {
		// Prompt user for input
		printf(": ");
		getline(&inputString, &buflen, stdin);

		// Strip newline character from inputString
		if (strlen(inputString) > 1) {
			inputString[strcspn(inputString, "\n")] = 0;
		}
		
		// Check if input was a comment
		if (isComment(inputString, '#') == 0) {
			// Parse input
			input = parseInput(inputString);

			if (strcmp(input->cmd, "exit") == 0) {
				return 0;
			}

			// Run command

			// Print output
		}


	}
	return 0;
}