// Assignment #3: smallsh
// Filename: smallsh.c
// Author: Paul Scrugham
// ONID #: 934308765
// Date Due: 2/7/2022

#include "smallsh.h"

int main(void)
{
	char* prompt = ": ";
	char* oldVar = "$$";
	char* inputString = NULL;
	char* exitStatusMessage = "exit value";
	char* termStatusMessage = "terminated by signal";
	int sentinel = 1;
	int fgStatus = -1;
	int bgStatus = -1;
	size_t buflen = 0;
	pid_t childPid;
	struct bgChildPIDs* bgChildList = NULL;


	// Initialize and install a signal handler for SIGINT
	struct sigaction SIGINT_action = { {0} };
	SIGINT_action.sa_handler = SIG_IGN;
	sigemptyset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = SA_RESTART;
	sigaction(SIGINT, &SIGINT_action, NULL);

	// Initialize and install a signal handler for SIGTSTP
	struct sigaction SIGTSTP_action = { {0} };
	SIGTSTP_action.sa_handler = sigtstpOn;
	sigemptyset(&SIGTSTP_action.sa_mask);
	sigaddset(&SIGTSTP_action.sa_mask, SIGTSTP);
	SIGTSTP_action.sa_flags = SA_RESTART;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

	while (sentinel) {
		// Non-blocking wait for background processes
		waitBG(childPid, bgStatus, bgChildList, exitStatusMessage, termStatusMessage);

		// Prompt user for input
		printf(prompt);
		fflush(stdout);
		fflush(stdin);
		getline(&inputString, &buflen, stdin);

		// Strip newline character generated by getline
		inputString[strcspn(inputString, "\n")] = 0;

		// After input is received, block SIGTSTP until foreground process has terminated
		sigprocmask(SIG_BLOCK, &SIGTSTP_action.sa_mask, NULL);

		// Check for empty or commented input string
		if (preParser(inputString) == 0) {
			continue;
		}

		// Declare the user input struct
		struct userInput* input;

		// Parse the input string for the command, arguments, I/O redirection, and background ps
		int numVars = countStringInstances(oldVar, inputString);
		if (numVars > 0) {
			// Expand any occurrences of "$$" to the program's PID
			char* expandedInput = expandVar(inputString, numVars, oldVar);
			input = parseInput(expandedInput);
			free(expandedInput);
		}
		else {
			input = parseInput(inputString);
		}

		// Reset background if foreground only mode is on
		if (flag == 1) {
			input->background = 0;
		}

		// Check if command is the built in "exit"
		if (strcmp(input->args[0], "exit") == 0) {
			// Clear background flag
			input->background = 0;
			// Run command
			sentinel = exitBuiltIn(bgChildList, bgStatus);
		}
		// Check if command is the built in "cd"
		else if (strcmp(input->args[0], "cd") == 0) {
			// Clear background flag
			input->background = 0;
			// Run command
			cdBuiltIn(input->args[1]);
		}
		// Check if command is the built in "status"
		else if (strcmp(input->args[0], "status") == 0) {
			// Clear background flag
			input->background = 0;
			// Run command
			statusBuiltIn(fgStatus, exitStatusMessage, termStatusMessage);
		}
		// Run an arbitrary command
		else {
			childPid = runArbitrary(input);
			// Handle foreground execution
			if (!input->background) {
				waitpid(childPid, &fgStatus, 0);
				
				// Print termination status if terminated by SIGINT
				if (WIFSIGNALED(fgStatus)) {
					printf("%s %d\n", termStatusMessage, WTERMSIG(fgStatus));
					fflush(stdout);
				}
			}
			// Handle background execution
			else {
				printf("background pid is %d\n", childPid);
				fflush(stdout);
				if (bgChildList) {
					// Add child to current list
					bgChildList = addChildPID(childPid, bgChildList);
				}
				else {
					// Start list
					bgChildList = createChildPID(childPid);
				}
			}
		}

		// After foreground processes have finished running, unblock SIGTSTP
		sigprocmask(SIG_UNBLOCK, &SIGTSTP_action.sa_mask, NULL);

		// free allocated memory at each loop iteration
		freeInputStruct(input);
	}
	
	// Free allocated heap memory
	freeChildPIDList(bgChildList);
	free(inputString);
	
	// If sentinel is still 1, the shell did not exit via the "exit" command
	if (sentinel) {
		return EXIT_FAILURE;
	}
	// The built in "exit" command sets sentinel to 0, indicating a successful exit
	else {
		return EXIT_SUCCESS;
	}
}