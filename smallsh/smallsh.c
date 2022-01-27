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

	while (1) {
		// Prompt user for input
		printf(": ");
		getline(&inputString, &buflen, stdin);

		// Parse input
		input = parseInput(inputString);

		// Run command

		// Print output
	}

}