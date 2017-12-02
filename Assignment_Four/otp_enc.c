#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <fcntl.h>
#include <sys/stat.h>

//NON MACRO GLOBALS
char* PROGNAME = "ENC_CLIENT";

//Struct for passing file data around
struct FileInfoObject{
	int KeyDescriptor;
	int TextDescriptor;

	int FileLength;// Key and Text must be the same length

	char* KeyFileName;
	char* TextFileName;
};
typedef struct FileInfoObject FileInfoObject;

//Brewster error function.
void error(const char *msg) 
{ 
	perror(msg); exit(0); // Error function used for reporting issues
} 

/// NAME: SpecificError
/// DESC: My error function that prints program name at beginning.
void SpecificError(const char* msg) 
{
	fprintf(stderr,"%s: %s\n",PROGNAME,msg);
	exit(1);
}

/// NAME: validArgc
/// DESC: checks that atleast 3 args are passed to the function.
void validArgc(int argc)
{
    if(argc == 3){
        SpecificError("Invalid number of arguments.");
        exit(1);
    }
}

/// NAME: InitEncryptionObject
/// DESC: creates an encryption object to store file data.
FileInfoObject* InitEncryptionObject(char** argv)
{
	FileInfoObject* file_ob = malloc(1 * sizeof(FileInfoObject));

	// set file names.
	file_ob->TextFileName = argv[1];
	file_ob->KeyFileName = argv[2];

	// open text file to get descriptor.
	file_ob->TextDescriptor = open(argv[1],O_RDONLY);
	if(file_ob->TextDescriptor < 0){
		SpecificError("Couldnt open plaintext file");
	}
	// open key file to get descriptor.
	file_ob->KeyDescriptor = open(argv[2],O_RDONLY);
	if(file_ob->KeyDescriptor < 0){
		SpecificError("Couldnt open keytext file");
	}

	return file_ob;
}

/// NAME: ValidatefileContent
/// DESC: checks for invalid chars in a file.
char* ValidatefileContent(FileInfoObject* Obj, char fd)
{
	int i,FileDescriptor;
	char* fileContent = malloc( Obj->FileLength * sizeof(char*));
	int Buffer;

	//conditionsal for which file descriptor we are using.
	if(fd == 'K'){
		FileDescriptor = Obj->KeyDescriptor;
	}
	else if(fd == 'T'){
		FileDescriptor = Obj->TextDescriptor;
	}

	//check if file can be opened.
	if(read(FileDescriptor,fileContent,Obj->FileLength) < 0){ // redundant.
		SpecificError("Couldnt open plaintext file");
	}

	// validate the contents of file is within A-Z or is a space.
	for(i = 0; i < Obj->FileLength; i++){
		Buffer = (fileContent[i]);
		if( !(Buffer == ' ' || (Buffer >= 'A' && Buffer <= 'Z')) ){
			SpecificError("Invalid character in a file.");
		}

	}
	// return an address to it in the heap.
	return fileContent;
}

/// NAME: CloseEncrytionObjFD
/// DESC: Clean up function for file descriptors.
void CloseEncrytionObjFD(FileInfoObject* Obj)
{
	// close desciptors.
	close(Obj->TextDescriptor);
	close(Obj->KeyDescriptor);

	//set to invalid num to be overwritten.
	Obj->TextDescriptor = -1;
	Obj->KeyDescriptor = -1;
}

/// NAME: GetEncryptionObjFileLength
/// DESC: checks if key file is longer than text then sets struct file length.
int GetEncryptionObjFileLength(FileInfoObject* Obj)
{
	struct stat Key,Text;

	// fail safe for stat error.
	if(stat(Obj->KeyFileName,&Key) < 0){
		SpecificError("Error getting stats of Keyfile.");
	}
	// fail safe for stat error.
	if(stat(Obj->TextFileName,&Text) < 0){
		SpecificError("Error getting stats of Textfile.");
	}

	//return error if key is shorter than text file.
	if(Key.st_size -1 < Text.st_size -1){
		SpecificError("KeyFile too short.");
	}
	else{
		// dont worry about null char.
		Obj->FileLength = Text.st_size -1;
	}

	return Obj->FileLength;
}

int main(int argc, char* argv[])
{
	//vars
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[256];
    
	//validate args and prep fileinfo struct.
	validArgc(argc);
	FileInfoObject* FileInfo = InitEncryptionObject(argv);
	GetEncryptionObjFileLength(FileInfo);

	//Read Key and TextFile.
	char* KeyText = ValidatefileContent(FileInfo,'K');
	char* RecievedText = malloc( FileInfo->FileLength * sizeof(char*));
	char* PlainText= ValidatefileContent(FileInfo,'T');
	char serverAccept;
	char fileLength[128];

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number

	// only connections on your current machine.
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { 
		fprintf(stderr, "CLIENT: ERROR, no such host\n");
		exit(1);
	}
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0){ 
		error("CLIENT: ERROR opening socket");
	}
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){ // Connect socket to address
	 	error("CLIENT: ERROR connecting");
	}


	//Authenticating server.
	send(socketFD,&(PROGNAME[0]), sizeof(char),0);//send client info E for Encryption client.
	recv(socketFD,&serverAccept,sizeof(char),0);// get Y or N for matching server.
	if(serverAccept != 'Y'){
		close(socketFD); // if not valid exit.
		SpecificError("not an Encryption server.");
	}

	//sending data to server.
	send(socketFD,&(FileInfo->FileLength), sizeof(FileInfo->FileLength),0);//send int of file length.
	send(socketFD,KeyText,FileInfo->FileLength * sizeof(char),0); // send key text.
	send(socketFD,PlainText,FileInfo->FileLength * sizeof(char),0); // send mesage text.

	//recieving data from server.
	recv(socketFD,RecievedText,FileInfo->FileLength * sizeof(char),0); //retrive encrpyted text.
	printf("%s\n",RecievedText);

	//freeing data.
	CloseEncrytionObjFD(FileInfo);
	free(KeyText);
	free(PlainText);
	free(RecievedText);
	free(FileInfo);

	return 0;
}
