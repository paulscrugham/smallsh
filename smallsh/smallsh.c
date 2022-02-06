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
	const char message[] = "\nEntering foreground-only mode (& is now ignored)";
	write(STDOUT_FILENO, message, sizeof message - 1);
	signal(SIGTSTP, &sigtstpOff);
}

void sigtstpOff(int signo) {
	flag = 0;
	const char message[] = "\nExiting foreground-only mode";
	write(STDOUT_FILENO, message, sizeof message - 1);
	signal(SIGTSTP, &sigtstpOn);
}

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

	// Initialize and install a signal handler for SIGINT
	struct sigaction SIGINT_action = { {0} };
	SIGINT_action.sa_handler = SIG_IGN;
	sigemptyset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = SA_RESTART;
	sigaction(SIGINT, &SIGINT_action, NULL);

	// Initialize and install a signal handler for SIGTSTP
	signal(SIGTSTP, &sigtstpOn);
	sigset_t sigtstpMask;
	sigemptyset(&sigtstpMask);
	sigaddset(&sigtstpMask, SIGTSTP);

	while (sentinel) {
		char* inputString = NULL;
		size_t buflen = 0;
		
		// Catch any background processes and print exit/termination status
		while ((childPid = waitpid(-1, &bgStatus, WNOHANG)) > 0) {
			printf("background pid %d is done: ", childPid);
			if (WIFEXITED(bgStatus)) {
				printf("%s %d\n", exitStatusMessage, WEXITSTATUS(bgStatus));
			}
			else if (WIFSIGNALED(bgStatus)) {
				printf("%s %d\n", termStatusMessage, WTERMSIG(bgStatus));
			}
			fflush(stdout);
		}

		// TODO: move declaration down to parseinput function
		struct userInput* input;

		// Prompt user for input
		printf(prompt);
		fflush(stdout);
		getline(&inputString, &buflen, stdin);

		// After input is received, block SIGTSTP until foreground process has terminated
		sigprocmask(SIG_BLOCK, &sigtstpMask, NULL);

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

		// Reset background if foreground only mode is on
		if (flag == 1) {
			input->background = 0;
		}

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
			}
			else if (WIFSIGNALED(fgStatus)) {
				printf("%s %d\n", termStatusMessage, WTERMSIG(fgStatus));
			}
			fflush(stdout);
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
				}
			}
			else {
				printf("background pid is %d\n", childPid);
			}
			fflush(stdout);
		}

		// After foreground processes have finished running, unblock SIGTSTP
		sigprocmask(SIG_UNBLOCK, &sigtstpMask, NULL);

		// Print output
		free(inputString);
	}
	// TODO: cleanup running background processes

	return 0;
}