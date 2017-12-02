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
typedef enum {FALSE = 0, TRUE = 1} boolean;

//Taken from assignment3
struct PidStackObj
{
    int NumBackPid;
    pid_t BackgroundPids[STACKMAX];
};

//NON MACRO GLOBALS
struct PidStackObj PidStack;
char* PROGNAME = "ENC_SERVER";
char AcceptedClientType = 'E';

/// NAME: _InitPidObj
/// DESC: Creates pid stack with -1 in each val.
/// SOURCE: assignment3
void _InitPidObj()
{
    int i;
    PidStack.NumBackPid = -1;

    for(i = 0; i < STACKMAX -1; i++){
        PidStack.BackgroundPids[i] = -1;
    }
}

/// NAME: PushBackPid
/// DESC: adds a pid to the stack
/// SOURCE: assignment3
void PushBackPid(pid_t processId)
{
    PidStack.BackgroundPids[++(PidStack.NumBackPid)] = processId;
}

/// NAME: PopBackPid
/// DESC: removes pid from top
/// SOURCE: assignment3
pid_t PopBackPid()
{
    return PidStack.BackgroundPids[PidStack.NumBackPid--];
}

/// NAME: TopBackPid
/// DESC: reads top of stack
/// SOURCE: assignment3
pid_t TopBackPid()
{
    return PidStack.BackgroundPids[PidStack.NumBackPid];
}

/// NAME: KillBGProcesses
/// DESC: helper function for exting.
/// SOURCE: assignment3
void TerminateServer(int sig)
{
    int i;
    for(i = 0;i < PidStack.NumBackPid + 1;i++){
        kill(PidStack.BackgroundPids[i], SIGINT); // interrupt all bg pids.
    }
}

//Brewster error function.
void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

/// NAME: SpecificError
/// DESC: My error function that prints program name at beginning.
void SpecificError(const char* msg) 
{
	fprintf(stderr,"%s: %s\n",PROGNAME,msg);
	exit(1);
}

/// NAME: Encryption
/// DESC: encrypt a text with a key.
char* Encryption(char* Key,char* Text)
{
	int i;
	int keytemp,texttemp;
	int length = strlen(Text);
	char EncryptionStr[70000];
	memset(EncryptionStr,'\0',sizeof(EncryptionStr));//setup buffer

	for(i = 0;i < length; i++){//iterate through all chars.
		if(Text[i] == ' '){
			EncryptionStr[i] = '?'; // replace spaces with ?
		}
		else{
			keytemp = (int)Key[i];
			texttemp = (int)Text[i];
			// take char int values and add them.
			EncryptionStr[i] = (char)(texttemp + (keytemp % 3));
		}
	}

	//return new encryption.
	return strdup(EncryptionStr);
}

int main(int argc, char *argv[])
{
	//initialization.
	_InitPidObj();
	signal(SIGINT,TerminateServer);//handler to kill bg pids.

	//vars.
	char FileBufferKey[70000];
	char FileBuffertext[70000];
	char* EncryptionBuffer;
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

	while(TRUE)
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

				//check the client for 'E'ncrpytion type.
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
				recv(establishedConnectionFD,FileBufferKey, FileLength * sizeof(char),0);//keyfile.
				recv(establishedConnectionFD,FileBuffertext,FileLength * sizeof(char),0);//textfile.
				EncryptionBuffer = Encryption(FileBufferKey,FileBuffertext);//encrypt files.

				// printf(":::%s|\n",FileBufferKey);
				// printf(":::%s|\n",FileBuffertext);
				// printf(":::%s|\n",EncryptionBuffer);

				//send decrption text back to client and close descriptor.
				send(establishedConnectionFD,EncryptionBuffer,FileLength * sizeof(char),0);
				shutdown(establishedConnectionFD,2);

				exit(0);
			default://parent
				//add to 5 possible processes.
				PushBackPid(pid);
		}

	}


	return 0; 
}
