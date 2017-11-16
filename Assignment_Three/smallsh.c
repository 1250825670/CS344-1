/**
* Name: Alexander Miranda
* Assignment: Program 3 smallsh
* Due Date: 11/15/2017
* Course: CS 344
*/

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>

/* Global Constants begin */
#define MAX_ARGS 512
#define MAX_CHAR 2048
/* Global Constants end */

/* Forward declarations */
void main_shell();
void print_array (char ** arguments, int size);
static void sig_handler(int sig);
/* Forward declarations end */

/** 
* This is a pointer to Strings that list the env variables
*/
extern char **env_vars_arr;

/**
 * Struct used to track the pids running in the background
 * pids: Array of pids
 * size: How many pids are currently listed in the array
 * capacity: How many pids the array can hold without resizing
*/
struct pid_array {
	pid_t * pids; // array of process id's
	int size;
	int capacity;
};

// Make a single pid_array in global scope
struct pid_array bg_process_pids;

/**
 * Removes a specified pid from the process pid array
 * 
 * pid - The pid of the process that will be removed from the array
*/
void delete_bg_pid(pid_t pid) {
	int i, j;
	if (pid > 0) {
		for (i = 0; i < bg_process_pids.size; i++) {
			if (bg_process_pids.pids[i] == pid) {
				bg_process_pids.size--;
				for (j = i; j < bg_process_pids.size; j++)
					bg_process_pids.pids[j] = bg_process_pids.pids[j + 1];
			}
		}
	}
}

/**
 * Creates a dynamic array composed of pid_t, assigns it to a global var
*/
void init_bg_process_arr() {
	bg_process_pids.capacity = 10;
	bg_process_pids.size = 0;
	bg_process_pids.pids = malloc(sizeof(pid_t) * bg_process_pids.capacity );
}

/**
 * Checks if the process array has reached its capacity
 * 
 * returns boolean - boolean that is true if array if full and false otherwise
**/
bool process_array_is_full() {
	return (bg_process_pids.size >= bg_process_pids.capacity);
}

/**
 * Adds pid to the process array global and will dynamically resize as needed
 * 
 * pid - The pid of the new process being added
*/
void append_bg_pid(pid_t pid) {
	// Checks if array is full and resizes if true
	if (process_array_is_full()) {
		bg_process_pids.capacity *= 2;
		bg_process_pids.pids = realloc(bg_process_pids.pids, sizeof(pid_t) * bg_process_pids.capacity );
	}
	bg_process_pids.pids[bg_process_pids.size++] = pid;
}

/**
 * Checks if a process's pid is within the bg_process_pids array
 * 
 * pid - The pid of the process being looked up in the array
*/
bool is_bg_process_pid(pid_t pid) {
	int i;
	for (i = 0; i < bg_process_pids.size; i++) {
		if (bg_process_pids.pids[i] == pid)
			return true;
	}
	return false;
}

/**
 * Kill all processes (by issuing SIGKILL) referenced in the bg_process_pids array
*/
void term_all_child_process() {
	pid_t bg_process;
	int i;
	for (i = 0; i < bg_process_pids.size; i++) {
		kill(bg_process_pids.pids[i], SIGKILL); // kill all child processes
	}
}

/**
 * Changes the current working directory
 * 
 * dir - String of the directory name (possibly path) the user is changing into
*/
void change_dir(char * dir) {
	// Check if the directory mentioned can be changed into
	// output error if chdir fails
	if(!chdir(dir) == 0) {
		perror("cd");
	}
}

/**
 * Looks up to see if any child processes have exited. If true the exit status
 * and/or termination signal are printed which then continues to look for
 * other finished processes to then do the same. This is done recursively
*/
void lookup_all_bg_process() {
	// Cite: Slide 21 lecture 9, and the manpage for wait()
	// iterates through all the bg id's so that we don't print foreground process?
	pid_t bg_pid = -1;
	int i;
	int bg_exit_status;

	bg_pid = waitpid(-1, &bg_exit_status, WNOHANG);
	if (bg_pid != 0 && bg_pid != -1) {
		if (bg_pid > 0) {
			if (bg_exit_status !=0)
				printf("background pid %d is done: terminated by sig %d\n", bg_pid, bg_exit_status);
			else
				printf("background pid %d exited with code %d\n", bg_pid, bg_exit_status);
			delete_bg_pid(bg_pid);
		}
		// recursively check for additional processes, otherwise might overlook multiple finishing between prompts
		lookup_all_bg_process(); 
	}
}

/**
 * Cleans up the shell by killing all child processes and frees the pids array
 * in the bg_process_pids struct
*/
void term_shell() {
	term_all_child_process();
	free(bg_process_pids.pids);
	exit(0);
}

/**
 * The main REPL function for the smallsh program. This process reads 
 * and parses user text input and prints output.
*/
void main_shell() {
	// Stores the current exit status to display to the user when
	// prompted
	int foreground_exit_stat = 0;
	// String constant denoting the /dev/null path
	const char * devnull = "/dev/null";

	// While loop that runs the shell process
	while (true) {
		// Storing data to properly parse user input
		int arg_count = 0, 
			word_count = 0;
		char ** arguments = malloc(MAX_ARGS * sizeof(char *));
		char * words[MAX_ARGS + 1];
		char * command = NULL, * input_file = NULL, * output_file = NULL;
		char input[MAX_CHAR + 1];
		memset (input, '\0', MAX_CHAR);
		
		// Booleans to track redirection mode and background vs foreground
		bool background_mode = false, redir_input = false, redir_output = false;
		
		// 
		struct sigaction act;
		act.sa_handler = SIG_IGN;
		sigaction(SIGINT, &act, NULL);

		// Looks up all running background processes
		lookup_all_bg_process();

		// Prompt and then read user provided input
		printf(": ");
		fflush(stdout);
		fflush(stdin);
		fgets(input, MAX_CHAR, stdin);

		if (strlen(input) > 1) {
			// Check if lines entered are comments which are prepended
			// with octothrop
			if (input[0] == '#') {
				continue;
			}
			input[strlen(input)-1] = '\0';
		}

		// Check that user input is non-null
		if (command = strtok(input, " \n")) {
			// store command as arguments[0] then move forward to tokenize rest
			arguments[arg_count++] = command;

			// Tokenized user input without needing to check if syntax errors occur
			while(words[word_count] = strtok(NULL, " ")) {
				char * word = words[word_count]; // Vastly improves readability in this block
				
				// End while loop if pound sign (#) is first char in input entered
				if (word[0] == '#') {
					word[0] = '\0';
					break;
				}

				// set input redirection mode and filename
				if (strcmp(word, "<") == 0) {
					words[++word_count] = strtok(NULL, " ");
					input_file = words[word_count];
					redir_input = true;
				}
				
				// set output redirectino mode and filename
				else if (strcmp(word, ">") == 0) {
					words[++word_count] = strtok(NULL, " ");
					output_file = words[word_count];
					redir_output = true;
				}

				// Set BG mode, stop reading rest of line (must be last argument)
				else if (strcmp(word, "&") == 0) {
					background_mode = true;
					break;
				}
				
				// all others are arguments (we assume user does correct syntax)
				else {
					arguments[arg_count++] = words[word_count++];
				}
			}

			// Adding a NULL entry to the end of the arguments array so the
			// end of the arguments array
			arguments[arg_count] = NULL;
			
			// Checks if the user enters exit and if so will
			// terminate the main shell process
			if (strcmp(command, "exit") == 0) {
				free(arguments);
				term_shell();
			}

			// Will run the cd command and change to the directory provided
			// if no directory/path is provided the directory will change
			// to the HOME variable defined in the environment
			else if (strcmp(command, "cd") == 0) {
				// If only argument is the 'cd' command, change to HOME path variable directory
				if (arg_count == 1) {
					// Changes to the directory set as HOME in environmental variables
					change_dir(getenv("HOME"));
				}

				// if a path/directory was passed by the user attempt to change 
				// to that directory
				else if (arg_count == 2) {
					change_dir(arguments[1]);
				}

				// else more arguments were provided, print usage message.
				else {
					printf("smallsh: cd: usage: cd [directory]\n");
				}
			}

			// Outputs the exit status of the last foreground process that finished
			else if (strcmp(command, "status") == 0) {
				// Outputs the exit status to the user
				printf("exit value %d\n", foreground_exit_stat);
			}

			// This branch will fork processes into their own child processes
			else {
				// fork process, then set up in/out redirection as appropriate
				pid_t pid = fork(); // Parent process gets pid of child assigned, child gets 0
				pid_t wpid;
				int fd_in, fd_out, fd_in2, fd_out2;

				// Execute process if a child process
				if (pid == 0) {
					
					if (!redir_input && background_mode)
						input_file = "/dev/null";
					if (!redir_output && background_mode)
						output_file = "/dev/null";

					// Open input_file if not null (if it's null, then some file was provided or its foreground)
					if (input_file) {
						fd_in = open(input_file, O_RDONLY);
						if (fd_in == -1) {
							perror("open");
							exit(1);
						}
						fd_in2 = dup2(fd_in, 0); // 0 = stdin
						if (fd_in2 == -1) {
							perror("dup2");
							exit(2);
						}
					}

					// Open output_file if not null (if it's null, then some file was provided or its foreground)
					if (output_file) {
						fd_out = open(output_file, O_WRONLY | O_TRUNC | O_CREAT, 0644);
						if (fd_out == -1) {
							perror("open");
							exit(1);
						}

						fd_out2 = dup2(fd_out, 1); // 1 = stdout
						if (fd_out2 == -1) {
							perror("dup2");
							exit(2);
						}					
					}

					// Intercepts the SIGINT signal if the process is not in the background
					if (!background_mode) {
						act.sa_flags = 0;
						act.sa_handler = SIG_DFL;
						sigaction(SIGINT, &act, NULL);
					}

					foreground_exit_stat = execvp(command, arguments);

					// Checks if the status denotes an error and outputs that
					// error to the user
					if(foreground_exit_stat == -1) {
						perror(command);
						foreground_exit_stat = 1;
					}
					exit(EXIT_FAILURE); 
				} // end child process
				
				// fork failed
				else if (pid == -1) {
					perror("fork()");
				}

				// Main process branch
				else {
					// Checks if the process was placed into the background
					// if true the pid will be tracked but do not wait
					if (background_mode) {
						append_bg_pid(pid);
						printf("background pid is %d\n", pid);
					}
					else {
						// Wait until the process finishes or is terminated
	               		pid = waitpid(pid, &foreground_exit_stat, 0);

						if (pid != -1 && pid != 0) {
							if (WIFSIGNALED(foreground_exit_stat)) {
								foreground_exit_stat = WTERMSIG(foreground_exit_stat);
								printf("pid %d is done: terminated by signal %d\n", pid, foreground_exit_stat);
							}
							else if(WIFEXITED(foreground_exit_stat)) {
								foreground_exit_stat = WEXITSTATUS(foreground_exit_stat);
							}
						}
			        }
				}
			}
		}
		// Deallocates the memory assigned to the arguments array
		if(arguments) free(arguments);	
	}
}

/**
 * Initializes the bg_process_arr global and starts the main_shell process
 * 
 * argc - Number of command-line arguments passed to smallsh execution
 * argv - Array of char arrays representing the arguments passed to smallsh
 * 
 * return integer - Returns 0 showing successfuly execution
*/
int main (int argc, char const *argv[])
{
	// Initialize the bg_process_arr global that tracks the background process pids
	init_bg_process_arr();

	main_shell();
	return 0;
}