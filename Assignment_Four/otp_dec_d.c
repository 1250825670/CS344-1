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

//Taken from assignment3
struct pid_tracker
{
    int pid_num;
    pid_t background_pids[MAX_STACK_LENGTH];
};

//NON MACRO GLOBALS
struct pid_tracker pid_stack;
char * PROGRAM_NAME = "DEC_SERVER";
char AcceptedClientType = 'D';

/// NAME: init_pid_stack
/// DESC: Creates pid stack with -1 in each val.
/// SOURCE: assignment3
void init_pid_stack()
{
    int i;
    pid_stack.pid_num = -1;

    for(i = 0; i < MAX_STACK_LENGTH - 1; i++){
        pid_stack.background_pids[i] = -1;
    }
}

/// NAME: push_pid
/// DESC: adds a pid to the stack
/// SOURCE: assignment3
void push_pid(pid_t processId)
{
    pid_stack.background_pids[++(pid_stack.pid_num)] = processId;
}

/// NAME: pop_pid
/// DESC: removes pid from top
/// SOURCE: assignment3
pid_t pop_pid()
{
    return pid_stack.background_pids[pid_stack.pid_num--];
}

/// NAME: peek_pid
/// DESC: reads top of stack
/// SOURCE: assignment3
pid_t peek_pid()
{
    return pid_stack.background_pids[pid_stack.pid_num];
}

/// NAME: KillBGProcesses
/// DESC: helper function for exting.
/// SOURCE: assignment3
void kill_server(int sig)
{
    int i;
    for(i = 0;i < pid_stack.pid_num + 1;i++){
        kill(pid_stack.background_pids[i], SIGINT); // interrupt all bg pids.
    }
}

//Brewster error function.
void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

/// NAME: SpecificError
/// DESC: My error function that prints program name at beginning.
void SpecificError(const char* msg) 
{
	fprintf(stderr,"%s: %s\n",PROGRAM_NAME,msg);
	exit(1);
}

/// NAME: decrypt_text
/// DESC: decrpt text with key text.
char * decrypt_text(char* Key,char* Text)
{
	int i;
	int keytemp,texttemp;
	int length = strlen(Text);
	char EncryptionStr[FILE_SIZE];
	memset(EncryptionStr,'\0',sizeof(EncryptionStr));//setup buffer

	for(i = 0;i < length; i++){//iterate through all chars.
		if(Text[i] == '?'){
			EncryptionStr[i] = ' ';// replace ? with spaces
		}
		else{
			keytemp = (int)Key[i];
			texttemp = (int)Text[i];
			// take char int values and subtract them.
			EncryptionStr[i] = (char)(texttemp - (keytemp % 3));
		}
	}

	//return new encryption.
	return strdup(EncryptionStr);
}

int main(int argc, char *argv[])
{
	//initialization.
	init_pid_stack();
	signal(SIGINT,kill_server);//handler to kill bg pids.

	//vars.
	char FileBufferKey[FILE_SIZE];
	char FileBuffertext[FILE_SIZE];
	char * EncryptionBuffer;
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
	if (listenSocketFD < 0){ 
		error("ERROR opening socket");
	}

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){ // Connect socket to port
		error("ERROR on binding");
	}
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections
	sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect

	while(1)
	{
		// Accept a connection, blocking if one is not available until one connects
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) error("ERROR on accept");

		pid_t pid = fork();
		switch(pid)
		{
			case -1:
				SpecificError("Child fork error.");
			case 0://child.

				//check the client for 'D'ecryption type.
				recv(establishedConnectionFD,&clienttype,sizeof(char),0);
				if(clienttype != AcceptedClientType){//if not matching client send no connection.
					charResponse = 'N';
					send(establishedConnectionFD,&charResponse,sizeof(char),0);
					SpecificError("Invalid client connection.");//error
				}
				else{
					//send accept client.
					charResponse = 'Y';
					send(establishedConnectionFD,&charResponse,sizeof(char),0);
				}

				recv(establishedConnectionFD,&FileLength,sizeof(FileLength),0);
				//printf(":::%d\n",FileLength);

				memset(FileBufferKey,'\0',sizeof(FileBufferKey));
				memset(FileBuffertext,'\0',sizeof(FileBuffertext));

				//begin reading in File
				recv(establishedConnectionFD,FileBufferKey, FileLength * sizeof(char),0);//keyfile
				recv(establishedConnectionFD,FileBuffertext,FileLength * sizeof(char),0);//textfile.
				EncryptionBuffer = decrypt_text(FileBufferKey,FileBuffertext);//decrpt files.

				//send encryption text back to client and close descriptor.
				send(establishedConnectionFD,EncryptionBuffer,FileLength * sizeof(char),0);
				shutdown(establishedConnectionFD,2);

				exit(0);
			default://parent
				//add to 5 possible processes.
				push_pid(pid);
		}

	}


	return 0; 
}
