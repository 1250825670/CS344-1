/**
 * Name: Alexander Miranda
 * Due Date: December 1st, 2017
 * Assignment: OTP (One Time Pad)
 * 
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#define MAX_STACK_LENGTH 5
#define FILE_SIZE 70000
#define MAX_BUFF 256

struct pid_tracker {
    int pid_num;
    pid_t background_pids[MAX_STACK_LENGTH];
};

// Globals start
struct pid_tracker pid_stack;
char * program_name = "DEC_SERVER";
char proper_client_type = 'D';
// Globals end

/**
 * Create the pid stack to track background processes setting each value to -1 initially
*/
void init_pid_stack() {
    int i;
    pid_stack.pid_num = -1;

    for(i = 0; i < MAX_STACK_LENGTH - 1; i++){
        pid_stack.background_pids[i] = -1;
    }
}

/**
 * Pushes a pid for a process onto the background_pids array (stack)
 * 
 * pid: {pid_t} - Pid for the process being pushed onto the stack
*/
void push_pid (pid_t pid) {
    pid_stack.background_pids[++(pid_stack.pid_num)] = pid;
}

/**
 * Remove the topmost/most recent pid from the background_pids array
 * 
 * returns: Returns the removed pid
*/
pid_t pop_pid() {
    return pid_stack.background_pids[pid_stack.pid_num--];
}

/**
 * Looks at the top of the pid stack
 * 
 * returns: {pid_t} - The topmost background pid
*/
pid_t peek_pid() {
    return pid_stack.background_pids[pid_stack.pid_num];
}

/**
 * Helper function that will kill the background processes that exist in the pid_stack
 * 
 * sig: The passed in signal number
*/
void kill_server(int sig) {
    int i;
    for(i = 0; i < pid_stack.pid_num + 1; i++) {
		// Send SIGINT signal to all background processes that were
		// started by this program
        kill(pid_stack.background_pids[i], SIGINT);
    }
}

/**
 * Professor provided error function
 * 
 * msg: Body of the error message to be outputted
*/
void error (const char * msg) { 
	perror(msg); 
	exit(1); 
}

/**
 * General error helper function that will output the error with program name
 * 
 * msg: The body of the error message to be outputted
*/
void err_helper(const char * msg) {
	fprintf(stderr, "%s: %s\n", program_name, msg);
	exit(1);
}

/**
 * Decrypts the text using the key provided
 * 
 * key: {char *} - The string representing the cipher key
 * text: {char *} - The string representing the text to encrypt
 * 
 * returns: {char *} - Returns a copy of the encrypted text blob
*/
char * decrypt_text(char * key, char * text) {
	int tmp_key;
	int tmp_text;
	int length = strlen(text);
	char encrypted_text[FILE_SIZE];
	int i;

	memset(encrypted_text, '\0', sizeof(encrypted_text));

	for(i = 0; i < length; i++) {
		if(text[i] == '?'){
			encrypted_text[i] = ' ';
		} else{
			tmp_key = (int) key[i];
			tmp_t ext = (int) text[i];
			encrypted_text[i] = (char)(tmp_text - (tmp_key % 3));
		}
	}

	return strdup(encrypted_text);
}

/**
 * Main routine for the otp_dec_d file
 * 
 * argc: {Integer} - The number of args provided to the executable
 * argv: {char * Array} - The char * Array representing the args passed to the executable
 * 
 * returns: {Integer} - Returns an integer representing success or failure
*/
int main (int argc, char * argv[]) {
	// Allocate and initialize pid tracking with stack
	init_pid_stack();
	// Listening for the SIGINT to run the kill_server callback
	signal(SIGINT, kill_server);

	char file_key_buff[FILE_SIZE];
	char file_text_buff[FILE_SIZE];
	char clienttype;
	char * encrypted_buff;
	int listening_sock_file_description;
	int connection_file_description;
	char response;
	int charsRead; 
	int file_len;
	int portNumber;

	socklen_t client_data_size;
	char buffer[MAX_BUFF];
	struct sockaddr_in serverAddress, clientAddress;

	if (argc < 2) { 
		fprintf(stderr,"USAGE: %s port\n", argv[0]); 
		exit(1); 
	} 

	memset((char *) &serverAddress, '\0', sizeof(serverAddress));
	// Parse out the port number
	portNumber = atoi(argv[1]);

	// Configure and create the socket connection
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(portNumber);
	listening_sock_file_description = socket(AF_INET, SOCK_STREAM, 0);

	if (listening_sock_file_description < 0) { 
		error("ERROR opening socket!");
	}

	// Bind the socket to begin listening to the port
	if (bind(listening_sock_file_description, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
		error("ERROR could not bind to the port!");
	}

	listen(listening_sock_file_description, MAX_STACK_LENGTH); // Flip the socket on - it can now receive up to 5 connections
	client_data_size = sizeof(clientAddress); // Get the size of the address for the client that will connect

	while(1) {
		// Attempts a connection followed by an error check if it fails
		connection_file_description = accept(listening_sock_file_description, (struct sockaddr *) &clientAddress, &client_data_size);
		if (connection_file_description < 0) {
			error("ERROR on accept!");
		}


		pid_t pid = fork();

		switch(pid) {
			case -1:
				err_helper("Child fork was not created");
			case 0:
				//check the client for 'D'ecryption type.
				recv(connection_file_description, &clienttype, sizeof(char), 0);
				if (clienttype != proper_client_type) {
					response = 'N';
					send(connection_file_description, &response, sizeof(char), 0);
					err_helper("Invalid client connection!");
				} else {
					response = 'Y';
					send(connection_file_description, &response, sizeof(char), 0);
				}

				recv(connection_file_description, &file_len, sizeof(file_len), 0);

				memset(file_key_buff, '\0', sizeof(file_key_buff));
				memset(file_text_buff, '\0', sizeof(file_text_buff));

				recv(connection_file_description, file_key_buff, file_len * sizeof(char), 0);
				recv(connection_file_description, file_text_buff, file_len * sizeof(char), 0);
				encrypted_buff = decrypt_text(file_key_buff, file_text_buff);

				send(connection_file_description, encrypted_buff, file_len * sizeof(char), 0);
				shutdown(connection_file_description, 2);

				exit(0);
			default:
				push_pid(pid);
		}
	}

	return 0; 
}
