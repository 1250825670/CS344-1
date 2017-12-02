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

struct pid_tracker {
    int pid_num;
    pid_t background_pids[MAX_STACK_LENGTH];
};

// Globals start
struct pid_tracker pid_stack;
char * PROGRAM_NAME = "DEC_SERVER";
char AcceptedClientType = 'D';
// Globals end

/// NAME: init_pid_stack
/// DESC: Creates pid stack with -1 in each val.
/// SOURCE: assignment3
void init_pid_stack() {
    int i;
    pid_stack.pid_num = -1;

    for(i = 0; i < MAX_STACK_LENGTH - 1; i++){
        pid_stack.background_pids[i] = -1;
    }
}

/// NAME: push_pid
/// DESC: adds a pid to the stack
/// SOURCE: assignment3
/**
 * 
*/
void push_pid(pid_t processId) {
    pid_stack.background_pids[++(pid_stack.pid_num)] = processId;
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
	fprintf(stderr, "%s: %s\n", PROGRAM_NAME, msg);
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

int main(int argc, char *argv[]) {
	// Allocate and initialize pid tracking with stack
	init_pid_stack();
	signal(SIGINT, kill_server);//handler to kill bg pids.

	char FileBufferKey[FILE_SIZE];
	char FileBuffertext[FILE_SIZE];
	char * EncryptionBuffer;
	char clienttype,charResponse;
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead ,FileLength;
	socklen_t sizeOfClientInfo;
	char buffer[256];
	struct sockaddr_in serverAddress, clientAddress;

	if (argc < 2) { 
		fprintf(stderr,"USAGE: %s port\n", argv[0]); 
		exit(1); 
	} 
	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0){ 
		error("ERROR opening socket");
	}

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){ // Connect socket to port
		error("ERROR on binding");
	}
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections
	sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect

	while(1) {
		// Accept a connection, blocking if one is not available until one connects
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
		if (establishedConnectionFD < 0) error("ERROR on accept");

		pid_t pid = fork();
		switch(pid) {
			case -1:
				err_helper("Child fork error.");
			case 0://child.

				//check the client for 'D'ecryption type.
				recv(establishedConnectionFD,&clienttype,sizeof(char),0);
				if(clienttype != AcceptedClientType){//if not matching client send no connection.
					charResponse = 'N';
					send(establishedConnectionFD,&charResponse,sizeof(char),0);
					err_helper("Invalid client connection.");//error
				} else {
					//send accept client.
					charResponse = 'Y';
					send(establishedConnectionFD,&charResponse,sizeof(char),0);
				}

				recv(establishedConnectionFD,&FileLength,sizeof(FileLength),0);

				memset(FileBufferKey,'\0',sizeof(FileBufferKey));
				memset(FileBuffertext,'\0',sizeof(FileBuffertext));

				recv(establishedConnectionFD,FileBufferKey, FileLength * sizeof(char),0);
				recv(establishedConnectionFD,FileBuffertext,FileLength * sizeof(char),0);
				EncryptionBuffer = decrypt_text(FileBufferKey,FileBuffertext);

				send(establishedConnectionFD,EncryptionBuffer,FileLength * sizeof(char),0);
				shutdown(establishedConnectionFD,2);

				exit(0);
			default:
				push_pid(pid);
		}
	}

	return 0; 
}
