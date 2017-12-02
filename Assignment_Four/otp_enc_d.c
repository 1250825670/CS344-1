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
#define STACKMAX 5


struct pid_stack
{
    int NumBackPid;
    pid_t BackgroundPids[STACKMAX];
};

// Globals start
struct pid_stack pid_stack;
char * PROGRAM_NAME = "ENC_SERVER";
char accepted_client_type = 'E';
// Globals end

/// NAME: init_pid_stack
/// DESC: Creates pid stack with -1 in each val.
/// SOURCE: assignment3
void init_pid_stack() {
    int i;
    pid_stack.NumBackPid = -1;

    for(i = 0; i < STACKMAX -1; i++){
        pid_stack.BackgroundPids[i] = -1;
    }
}

/// NAME: PushBackPid
/// DESC: adds a pid to the stack
/// SOURCE: assignment3
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
void err_helper(const char* msg) 
{
	fprintf(stderr, "%s: %s\n", PROGRAM_NAME, msg);
	exit(1);
}

/// NAME: encrypt
/// DESC: encrypt a text with a key.
/**
 * 
*/
char * encrypt_text(char * key, char * text) {
	int i;
	int tmp_key;
	int tmp_text;
	int length = strlen(text);
	char encryptStr[70000];
	memset(encryptStr, '\0', sizeof(encryptStr));

	for (i = 0; i < length; i++) {
		if(text[i] == ' ') {
			encryptStr[i] = '?';
		} else {
			tmp_key = (int) key[i];
			tmp_text = (int) text[i];
			encryptStr[i] = (char) (tmp_text + (tmp_key % 3));
		}
	}

	return strdup(encryptStr);
}

int main (int argc, char * argv[]) {
	init_pid_stack();
	signal(SIGINT, kill_server);

	char FileBufferKey[70000];
	char FileBuffertext[70000];
	char* encryptBuffer;
	char clienttype,charResponse;
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead ,FileLength;
	socklen_t sizeOfClientInfo;
	char buffer[256];
	struct sockaddr_in serverAddress, clientAddress;

	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) { 
		error("ERROR opening socket");
	}

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) { // Connect socket to port
		error("ERROR on binding");
	}
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections
	sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect

	while(1) {
		// Accept a connection, blocking if one is not available until one connects
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) error("ERROR on accept");

		pid_t pid = fork();
		switch(pid) {
			case -1:
				err_helper("Child fork error.");
			case 0:
				recv(establishedConnectionFD, &clienttype, sizeof(char), 0);
				if (clienttype != accepted_client_type) {
					charResponse = 'N';
					send(establishedConnectionFD, &charResponse, sizeof(char), 0);
					err_helper("Invalid client connection.");
				} else {
					charResponse = 'Y';
					send(establishedConnectionFD,&charResponse,sizeof(char),0);
				}

				recv(establishedConnectionFD,&FileLength,sizeof(FileLength),0);

				memset(FileBufferKey,'\0', sizeof(FileBufferKey));
				memset(FileBuffertext,'\0', sizeof(FileBuffertext));

				recv(establishedConnectionFD, FileBufferKey, FileLength * sizeof(char), 0);
				recv(establishedConnectionFD, FileBuffertext, FileLength * sizeof(char), 0);
				encryptBuffer = encrypt_text(FileBufferKey, FileBuffertext);

				
				send(establishedConnectionFD, encryptBuffer, FileLength * sizeof(char), 0);
				shutdown(establishedConnectionFD, 2);

				exit(0);
			default://parent
				//add to 5 possible processes.
				PushBackPid(pid);
		}

	}


	return 0; 
}
