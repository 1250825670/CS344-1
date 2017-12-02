/**
 * Name: Alexander Miranda
 * Due Date: December 1st, 2017
 * Assignment: OTP (One Time Pad)
 * 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>

#define FILE_SIZE 70000
#define MAX_BUFF 256
#define MAX_STACK_LENGTH 5

struct pid_stack {
    pid_t background_pids[MAX_STACK_LENGTH];
	int pid_num;
};

// Globals start
struct pid_stack pid_stack;
char * PROGRAM_NAME = "ENC_SERVER";
char accepted_client_type = 'E';
// Globals end

/**
 * Create the pid stack to track background processes setting each value to -1 initially
*/
void init_pid_stack() {
    pid_stack.pid_num = -1;
	int i;

    for(i = 0; i < MAX_STACK_LENGTH - 1; i++){
        pid_stack.background_pids[i] = -1;
    }
}

/**
 * Pushes a pid number to the stack
 * 
 * pid: {pid_t} - The process's id to be added to the background pid stack
 * 
*/
void push_pid(pid_t pid) {
    pid_stack.background_pids[++(pid_stack.pid_num)] = pid;
}

/**
 * Helper function that will kill the background processes that exist in the pid_stack
 * 
 * sig: The passed in signal number
*/
void kill_server(int sig) {
    int i;
    for(i = 0;i < pid_stack.pid_num + 1;i++){
        kill(pid_stack.background_pids[i], SIGINT); // interrupt all bg pids.
    }
}

/**
 * Professor provided error function
 * 
 * msg: Body of the error message passed to perror
*/
void error(const char * msg) { 
	perror(msg); 
	exit(1); 
}

/**
 * Helper function that outputs errors with the program name prepended
 * 
 * msg: {const char *} - The body of the error message to output
*/
void err_helper(const char * msg) {
	fprintf(stderr, "%s: %s\n", PROGRAM_NAME, msg);
	exit(1);
}

/**
 * Encrypts a body of text using a passed in cipher key
 * 
 * key: {char *} - String representing the cipher key
 * text: {char *} - String representing the text that will be encrypted
 * 
 * returns: {char *} - A copy of the encrypted text body
*/
char * encrypt_text(char * key, char * text) {
	int i;
	int tmp_key;
	int tmp_text;
	int len = strlen(text);
	char encrypted_text[FILE_SIZE];

	memset(encrypted_text, '\0', sizeof(encrypted_text));

	for (i = 0; i < len; i++) {
		if(text[i] == ' ') {
			encrypted_text[i] = '?';
		} else {
			tmp_key = (int) key[i];
			tmp_text = (int) text[i];
			encrypted_text[i] = (char) (tmp_text + (tmp_key % 3));
		}
	}

	return strdup(encrypted_text);
}

/**
 * Main routine for the otp_enc_d file
 * 
 * argc: {Integer} - The number of args provided to the executable
 * argv: {char * Array} - The char * Array representing the args passed to the executable
 * 
 * returns: {Integer} - Returns an integer representing success or failure
*/
int main (int argc, char * argv[]) {
	init_pid_stack();
	signal(SIGINT, kill_server);

	char file_key_buff[FILE_SIZE];
	char file_text_buff[FILE_SIZE];
	char * encrypted_buff;
	int listener_sock_file_description;
	int connection_file_description;
	int port_num;
	char clienttype;
	char response;
	int charsRead;
	int file_len;
	socklen_t client_data_size;
	char buffer[MAX_BUFF];
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;

	// Instruct the user if they do not provide enough args
	if (argc < 2) { 
		fprintf(stderr, "USAGE: %s port\n", argv[0]);
		exit(1);
	}

	// Set up the address struct for this process (the server)
	memset((char *) &server_addr, '\0', sizeof(server_addr));
	port_num = atoi(argv[1]);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_num);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	// Set up the socket
	listener_sock_file_description = socket(AF_INET, SOCK_STREAM, 0);
	if (listener_sock_file_description < 0) { 
		error("ERROR opening socket");
	}

	if (bind(listener_sock_file_description, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		error("ERROR on binding");
	}

	listen(listener_sock_file_description, MAX_STACK_LENGTH);
	client_data_size = sizeof(client_addr);

	while(1) {
		// Attempts a connection followed by an error check if it fails
		connection_file_description = accept(listener_sock_file_description, (struct sockaddr *) &client_addr, &client_data_size);
		if (connection_file_description < 0) { 
			error("ERROR on accept");
		}

		pid_t pid = fork();

		switch(pid) {
			case -1:
				err_helper("Child fork was not created");
			case 0:
				// Validating client connection
				recv(connection_file_description, &clienttype, sizeof(char), 0);
				if (clienttype != accepted_client_type) {
					response = 'N';
					send(connection_file_description, &response, sizeof(char), 0);
					err_helper("Invalid client connection.");
				} else {
					response = 'Y';
					send(connection_file_description,&response,sizeof(char),0);
				}

				recv(connection_file_description,&file_len,sizeof(file_len),0);

				memset(file_key_buff, '\0', sizeof(file_key_buff));
				memset(file_text_buff, '\0', sizeof(file_text_buff));

				recv(connection_file_description, file_key_buff, file_len * sizeof(char), 0);
				recv(connection_file_description, file_text_buff, file_len * sizeof(char), 0);
				encrypted_buff = encrypt_text(file_key_buff, file_text_buff);

				
				send(connection_file_description, encrypted_buff, file_len * sizeof(char), 0);
				shutdown(connection_file_description, 2);

				exit(0);
			default:
				push_pid(pid);
		}
	}
	return 0; 
}
