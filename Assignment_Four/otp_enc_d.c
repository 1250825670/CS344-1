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
#define MAX_STACK_ENTRIES 5

struct pid_stack {
    pid_t BackgroundPids[MAX_STACK_ENTRIES];
	int NumBackPid;
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
    pid_stack.NumBackPid = -1;
	int i;

    for(i = 0; i < MAX_STACK_ENTRIES -1; i++){
        pid_stack.BackgroundPids[i] = -1;
    }
}

/**
 * Pushes a pid number to the stack
 * 
 * pid: {pid_t} - The process's id to be added to the background pid stack
 * 
*/
void PushBackPid(pid_t processId) {
    pid_stack.BackgroundPids[++(pid_stack.NumBackPid)] = processId;
}

/// NAME: PopBackPid
/// DESC: removes pid from top
/// SOURCE: assignment3
pid_t PopBackPid() {
    return pid_stack.BackgroundPids[pid_stack.NumBackPid--];
}

/// NAME: TopBackPid
/// DESC: reads top of stack
/// SOURCE: assignment3
pid_t TopBackPid() {
    return pid_stack.BackgroundPids[pid_stack.NumBackPid];
}

/// NAME: KillBGProcesses
/// DESC: helper function for exting.
/// SOURCE: assignment3
void kill_server(int sig) {
    int i;
    for(i = 0;i < pid_stack.NumBackPid + 1;i++){
        kill(pid_stack.BackgroundPids[i], SIGINT); // interrupt all bg pids.
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

/// NAME: err_helper
/// DESC: My error function that prints program name at beginning.
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
	char encrypted_string[FILE_SIZE];

	memset(encrypted_string, '\0', sizeof(encrypted_string));

	for (i = 0; i < len; i++) {
		if(text[i] == ' ') {
			encrypted_string[i] = '?';
		} else {
			tmp_key = (int) key[i];
			tmp_text = (int) text[i];
			encrypted_string[i] = (char) (tmp_text + (tmp_key % 3));
		}
	}

	return strdup(encrypted_string);
}

int main (int argc, char * argv[]) {
	init_pid_stack();
	signal(SIGINT, kill_server);

	char FileBufferKey[FILE_SIZE];
	char FileBuffertext[FILE_SIZE];
	char* encryptBuffer;
	char clienttype,charResponse;
	int listener_sock_file_description, connection_file_description, portNumber, charsRead ,FileLength;
	socklen_t sizeOfClientInfo;
	char buffer[256];
	struct sockaddr_in serverAddress, client_addr;

	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listener_sock_file_description = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listener_sock_file_description < 0) { 
		error("ERROR opening socket");
	}

	if (bind(listener_sock_file_description, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) { // Connect socket to port
		error("ERROR on binding");
	}

	listen(listener_sock_file_description, 5);
	sizeOfClientInfo = sizeof(client_addr);

	while(1) {
		connection_file_description = accept(listener_sock_file_description, (struct sockaddr *) &client_addr, &sizeOfClientInfo);
		if (connection_file_description < 0) { 
			error("ERROR on accept");
		}

		pid_t pid = fork();

		switch(pid) {
			case -1:
				err_helper("Child fork error.");
			case 0:
				recv(connection_file_description, &clienttype, sizeof(char), 0);
				if (clienttype != accepted_client_type) {
					charResponse = 'N';
					send(connection_file_description, &charResponse, sizeof(char), 0);
					err_helper("Invalid client connection.");
				} else {
					charResponse = 'Y';
					send(connection_file_description,&charResponse,sizeof(char),0);
				}

				recv(connection_file_description,&FileLength,sizeof(FileLength),0);

				memset(FileBufferKey,'\0', sizeof(FileBufferKey));
				memset(FileBuffertext,'\0', sizeof(FileBuffertext));

				recv(connection_file_description, FileBufferKey, FileLength * sizeof(char), 0);
				recv(connection_file_description, FileBuffertext, FileLength * sizeof(char), 0);
				encryptBuffer = encrypt_text(FileBufferKey, FileBuffertext);

				
				send(connection_file_description, encryptBuffer, FileLength * sizeof(char), 0);
				shutdown(connection_file_description, 2);

				exit(0);
			default:
				PushBackPid(pid);
		}
	}
	return 0; 
}
