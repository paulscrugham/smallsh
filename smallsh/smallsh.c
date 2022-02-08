// Assignment #3: smallsh
// Filename: smallsh.c
// Author: Paul Scrugham
// ONID #: 934308765
// Date Due: 2/7/2022

#include "smallsh.h"

volatile sig_atomic_t flag = 0;

void sigtstpOff(int);

void sigtstpOn(int signo) {
	flag = 1;
	const char message[] = "\nEntering foreground-only mode (& is now ignored)\n: ";
	write(STDOUT_FILENO, message, sizeof message - 1);
	signal(SIGTSTP, &sigtstpOff);
}

void sigtstpOff(int signo) {
	flag = 0;
	const char message[] = "\nExiting foreground-only mode\n: ";
	write(STDOUT_FILENO, message, sizeof message - 1);
	signal(SIGTSTP, &sigtstpOn);
}

int main(void)
{
	char* prompt = ": ";
	int sentinel = 1;
	pid_t childPid;
	int fgStatus = -1;
	int bgStatus;
	char* exitStatusMessage = "exit value";
	char* termStatusMessage = "terminated by signal";
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
		char* inputString = NULL;
		size_t buflen = 0;
		
		// Catch any background processes and print exit/termination status
		while ((childPid = waitpid(-1, &bgStatus, WNOHANG)) > 0) {
			printf("background pid %d is done: ", childPid);
			fflush(stdout);
			
			// Remove from bgChildList
			bgChildList = removeChildPID(childPid, bgChildList);
			
			// Update status
			if (WIFEXITED(bgStatus)) {
				printf("%s %d\n", exitStatusMessage, WEXITSTATUS(bgStatus));
				fflush(stdout);
			}
			else if (WIFSIGNALED(bgStatus)) {
				printf("%s %d\n", termStatusMessage, WTERMSIG(bgStatus));
				fflush(stdout);
			}
		}

		// TODO: move declaration down to parseinput function
		struct userInput* input;

		// Prompt user for input
		printf(prompt);
		fflush(stdout);
		fflush(stdin);
		sleep(1);
		getline(&inputString, &buflen, stdin);

		// After input is received, block SIGTSTP until foreground process has terminated
		sigprocmask(SIG_BLOCK, &SIGTSTP_action.sa_mask, NULL);

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
		char* oldVar = "$$";
		int numVars = countStringInstances(oldVar, inputString);
		
		if (numVars > 0) {
			char* expandedInput = expandVar(inputString, numVars, oldVar);
			input = parseInput(expandedInput);
			free(expandedInput);
		}
		else {
			input = parseInput(inputString);
		}
		free(inputString);

		// Reset background if foreground only mode is on
		if (flag == 1) {
			input->background = 0;
		}

		// Check if command is a smallsh built-in
		if (strcmp(input->args[0], "exit") == 0) {
			// Clear background
			input->background = 0;
			// Catch any background processes and terminate them
			struct bgChildPIDs* curr = bgChildList;
			while (curr) {
				// Terminate process
				kill(curr->pid, SIGTERM);
				// Clean up dead process
				waitpid(curr->pid, &bgStatus, WNOHANG);
				curr = curr->next;
			}

			return EXIT_SUCCESS;
		}
		else if (strcmp(input->args[0], "cd") == 0) {
			// Clear background
			input->background = 0;
			// Run command
			cdBuiltIn(input->args[1]);

		}
		else if (strcmp(input->args[0], "status") == 0) {
			// Clear background
			input->background = 0;
			if (fgStatus == -1) {
				printf("%s %d\n", exitStatusMessage, 0);
				fflush(stdout);
			}
			else if (WIFEXITED(fgStatus)) {
				printf("%s %d\n", exitStatusMessage, WEXITSTATUS(fgStatus));
				fflush(stdout);
			}
			else if (WIFSIGNALED(fgStatus)) {
				printf("%s %d\n", termStatusMessage, WTERMSIG(fgStatus));
				fflush(stdout);
			}
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
			else {
				printf("background pid is %d\n", childPid);
				fflush(stdout);
				// Add child PID to array
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

		// free the user input struct
		free(input);
	}
	
	return EXIT_FAILURE;
}