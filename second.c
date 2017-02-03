#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/**
 * Load image of a pre-built program and execute the loaded program.
 *
 * This is the very basic version of such action. Normally we should search through PATH
 * environment variable for the binary.
 */
void run(char **program)
{
	// Load program image and execute.
	// See more details: man execv
	execv(program[0], program);

	// Should never returns.
	fprintf(stderr, "Execv %s failed, %s.\n", program[0], strerror(errno));
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s <first program> <second program>\n", argv[0]);
		exit(0);
	}

	// Command of two programs.
	char **program1, **program2;
 
	// pipefd[0] is read end, pipefd[1] is write end.
	// See more details: man 2 pipe
	int pipefd[2];

	// Process ID for first and second programs.
	int pid1, pid2;

	// Set up command of two programs.
	program1 = calloc(2, sizeof(char*));
	program2 = calloc(2, sizeof(char*));
	program1[0] = argv[1];
	program2[0] = argv[2];

	// Create pipe for inter-process communication.
	if (-1 == pipe(pipefd))
	{
		fprintf(stderr, "Create pipe fail, %s\n", strerror(errno));
		exit(errno);
	}

	/*
	 * Create two child processes for running the two pre-built programs.
	 *
	 * The pipe is shared by both two child processes.
	 *
	 * The first child process (left side of pipe) only need the write end of pipe, in order
	 * to write data to the second process. The second child process (right side of pipe)
	 * only need the read end of pipe, in order to read data from the first process.
	 *
	 * They should close un-used end of pipe before load new process image.
	 */
	
	// Execute the first program.
	if (0 == (pid1 = fork()))
	{
		// The first program use stdandard input as its input.
		// Close the read end of pipe so that it won't suspend forever.
	    	if (-1 == close(pipefd[0])) {
	    	    	fprintf(stderr, "Close read end of pipe fail, %s\n", strerror(errno));
	    	    	exit(errno);
	    	}
	    	// Redirect write end of pipe to stdout so that the second program could read
		// data.
	    	if (-1 == dup2(pipefd[1], STDOUT_FILENO)) {
	    	    	fprintf(stderr, "Redirect file descriptor fails, %s\n", strerror(errno));
	    	    	exit(errno);
	    	}
	    	run(program1);
	}
	// Execute the second program.
	else if (0 == (pid2 = fork()))
	{
		// The second program use stdandard output as its output.
		// Close the write end of pipe so that it won't suspend forever.
	    	if (-1 == close(pipefd[1])) {
	    	    	fprintf(stderr, "Close write end of pipe fail, %s\n", strerror(errno));
	    	    	exit(errno);
	    	}
	    	// Redirect read end of pipe to stdin so that the first program could read
		// data.
	    	if (-1 == dup2(pipefd[0], STDIN_FILENO)) {
	    	    	fprintf(stderr, "Redirect file descriptor fails, %s\n", strerror(errno));
	    	    	exit(errno);
	    	}
	    	run(program2);
	}
	// The main program prints finish sentence.
	else
	{
		// Wait until two child processes finish.
		// See more details: man 2 wait
		waitpid(pid1, NULL, 0);
		waitpid(pid2, NULL, 0);

		printf("Run two programs succeeded!\n");
	}
}

