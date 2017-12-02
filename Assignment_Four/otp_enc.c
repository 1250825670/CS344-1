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
#include <netdb.h> 
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

// Globals start
char * PROGRAM_NAME = "ENC_CLIENT";
// Globals end

struct file_data_obj {
	int key;
	int text;
	int file_length;
	char * key_filename;
	char * text_filename;
};

typedef struct file_data_obj file_data_obj;

/**
 * Professor provided error function
 * 
 * msg: The body of the error message outputted
*/
void error(const char * msg) { 
	perror(msg); 
	exit(0);
} 

/**
 * Helper function that outputs error message with program name prepended
 * 
 * msg: Body of the error message
*/
void err_helper(const char * msg) {
	fprintf(stderr, "%s: %s\n", PROGRAM_NAME, msg);
	exit(1);
}

/// NAME: validArgc
/// DESC: checks that atleast 3 args are passed to the function.
void validArgc(int argc) {
    if(argc == 3){
        err_helper("Invalid number of arguments.");
        exit(1);
    }
}

/// NAME: InitEncryptionObject
/// DESC: creates an encryption object to store file data.
file_data_obj * InitEncryptionObject(char ** argv) {
	file_data_obj* file_ob = malloc(1 * sizeof(file_data_obj));

	// set file names.
	file_ob->text_filename = argv[1];
	file_ob->key_filename = argv[2];

	// open text file to get descriptor.
	file_ob->text = open(argv[1],O_RDONLY);
	if(file_ob->text < 0){
		err_helper("Couldnt open plaintext file");
	}
	// open key file to get descriptor.
	file_ob->key = open(argv[2],O_RDONLY);
	if(file_ob->key < 0){
		err_helper("Couldnt open keytext file");
	}

	return file_ob;
}

/// NAME: ValidatefileContent
/// DESC: checks for invalid chars in a file.
char * ValidatefileContent(file_data_obj * Obj, char fd) {
	int i,FileDescriptor;
	char* fileContent = malloc( Obj->file_length * sizeof(char*));
	int Buffer;

	//conditionsal for which file descriptor we are using.
	if(fd == 'K'){
		FileDescriptor = Obj->key;
	}
	else if(fd == 'T'){
		FileDescriptor = Obj->text;
	}

	//check if file can be opened.
	if(read(FileDescriptor,fileContent,Obj->file_length) < 0){ // redundant.
		err_helper("Couldnt open plaintext file");
	}

	// validate the contents of file is within A-Z or is a space.
	for(i = 0; i < Obj->file_length; i++){
		Buffer = (fileContent[i]);
		if( !(Buffer == ' ' || (Buffer >= 'A' && Buffer <= 'Z')) ){
			err_helper("Invalid character in a file.");
		}

	}
	// return an address to it in the heap.
	return fileContent;
}

/// NAME: CloseEncrytionObjFD
/// DESC: Clean up function for file descriptors.
void CloseEncrytionObjFD(file_data_obj * Obj) {
	// close desciptors.
	close(Obj->text);
	close(Obj->key);

	//set to invalid num to be overwritten.
	Obj->text = -1;
	Obj->key = -1;
}

/**
 * Finds the encrypted file's length and returns it
 * 
 * obj: {struct file_data_obj *} - Pointer to the file_data_obj that length will be called on
 * 
 * returns: {Integer} - The length of the file's length
*/
int get_encrypted_file_length(file_data_obj * file_obj) {
	struct stat key;
	struct stat text;

	// fail safe for stat error.
	if (stat(file_obj->key_filename, &key) < 0){
		err_helper("Error getting stats of Keyfile.");
	}
	// fail safe for stat error.
	if (stat(file_obj->text_filename, &text) < 0){
		err_helper("Error getting stats of Textfile.");
	}

	//return error if key is shorter than text file.
	if (key.st_size - 1 < text.st_size - 1){
		err_helper("KeyFile too short.");
	} else {
		// dont worry about null char.
		file_obj->file_length = text.st_size -1;
	}

	return file_obj->file_length;
}

int main(int argc, char * argv[]) {
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[256];
    
	validArgc(argc);
	file_data_obj* FileInfo = InitEncryptionObject(argv);
	get_encrypted_file_length(FileInfo);

	//Read Key and TextFile.
	char* KeyText = ValidatefileContent(FileInfo,'K');
	char* RecievedText = malloc( FileInfo->file_length * sizeof(char*));
	char* PlainText= ValidatefileContent(FileInfo,'T');
	char serverAccept;
	char file_length[128];

	// Set up the server address struct
	memset((char*) &serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number

	// only connections on your current machine.
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { 
		fprintf(stderr, "CLIENT: ERROR, no such host\n");
		exit(1);
	}
	memcpy((char*) &serverAddress.sin_addr.s_addr, (char*) serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0){ 
		error("CLIENT: ERROR opening socket");
	}
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0){ // Connect socket to address
	 	error("CLIENT: ERROR connecting");
	}

	//Authenticating server.
	send(socketFD,&(PROGRAM_NAME[0]), sizeof(char),0);//send client info E for Encryption client.
	recv(socketFD,&serverAccept,sizeof(char),0);// get Y or N for matching server.
	if(serverAccept != 'Y'){
		close(socketFD);
		err_helper("not an encryption server.");
	}

	//sending data to server.
	send(socketFD,&(FileInfo->file_length), sizeof(FileInfo->file_length),0);//send int of file length.
	send(socketFD,KeyText,FileInfo->file_length * sizeof(char),0); // send key text.
	send(socketFD,PlainText,FileInfo->file_length * sizeof(char),0); // send mesage text.

	//recieving data from server.
	recv(socketFD,RecievedText,FileInfo->file_length * sizeof(char),0); //retrive encrpyted text.
	printf("%s\n",RecievedText);

	//freeing data.
	CloseEncrytionObjFD(FileInfo);
	free(KeyText);
	free(PlainText);
	free(RecievedText);
	free(FileInfo);

	return 0;
}
