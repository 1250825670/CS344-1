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

#define BUFF_SIZE 256

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

/**
 * Generates a file encryption object to store the text data
 * 
 * argv: {char **} - Pointers to pointers of the arguments passed in
 * 
 * returns: {file_data_obj *} - Pointer to the populated file_data_obj
*/
file_data_obj * create_encryption_obj(char ** argv) {

	file_data_obj * file_obj = malloc(1 * sizeof(file_data_obj));

	// Populate the file names from passed in args
	file_obj->text_filename = argv[1];
	file_obj->key_filename = argv[2];

	// open text file to get descriptor.
	file_obj->text = open(argv[1], O_RDONLY);
	if(file_obj->text < 0){
		err_helper("Couldn't open plain text file!");
	}
	// open key file to get descriptor.
	file_obj->key = open(argv[2], O_RDONLY);
	if(file_obj->key < 0){
		err_helper("Couldnt open key_text file!");
	}

	return file_obj;
}

/**
 * Validates the text checking if there are unsupported chars
 * 
 * file_obj: {struct file_data_obj} - Pointer to the file_obj containing the text
 * file_descriptor: {char} - Char denoting if the validation is for the key or text on the file_obj
 * 
 * returns: {char *} - Returns pointer to the file content that was validated
*/
char * file_content_validate(file_data_obj * file_obj, char fd) {
	int i;
	int file_description;
	char * fileContent = malloc( file_obj->file_length * sizeof(char*));
	int buff;

	//conditionsal for which file descriptor we are using.
	if(fd == 'K'){
		file_description = file_obj->key;
	}
	else if(fd == 'T'){
		file_description = file_obj->text;
	}

	if(read(file_description,fileContent,file_obj->file_length) < 0){ // redundant.
		err_helper("Couldnt open raw_text file!");
	}

	for(i = 0; i < file_obj->file_length; i++){
		buff = (fileContent[i]);
		if( !(buff == ' ' || (buff >= 'A' && buff <= 'Z')) ){
			err_helper("Invalid character in a file!");
		}

	}

	// Returns a reference 
	return fileContent;
}

/**
 * Clear the data tied to the file_obj struct
 * 
 * file_obj: {struct file_data_obj} - The file_obj whose fields will be cleared
*/
void clear_encryption_obj(file_data_obj * file_obj) {
	// Close files tied to the file_obj
	close(file_obj->text);
	close(file_obj->key);

	// Assign dummy values to clear them
	file_obj->text = -1;
	file_obj->key = -1;
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

	if (stat(file_obj->key_filename, &key) < 0){
		err_helper("Error getting stats of key file!");
	}
	if (stat(file_obj->text_filename, &text) < 0){
		err_helper("Error getting stats of text file!");
	}
	if (key.st_size - 1 < text.st_size - 1){
		err_helper("Key file is too short!");
	} else {
		file_obj->file_length = text.st_size -1;
	}

	return file_obj->file_length;
}

/**
 * Main routine for the otp_enc file
 * 
 * argc: {Integer} - The number of args provided to the executable
 * argv: {char * Array} - The char * Array representing the args passed to the executable
 * 
 * returns: {Integer} - Returns an integer representing success or failure
*/
int main(int argc, char * argv[]) {
	int sock_file_description;
	int port_num;
	struct sockaddr_in server_addr;
	struct hostent * serverHostInfo;
	char buff[BUFF_SIZE];
    
	if (argc == 3) {
        err_helper("Invalid number of arguments.");
        exit(1);
    }

	file_data_obj * FileInfo = create_encryption_obj(argv);
	get_encrypted_file_length(FileInfo);

	char * key_text = file_content_validate(FileInfo,'K');
	char * response_text = malloc( FileInfo->file_length * sizeof(char*));
	char * raw_text= file_content_validate(FileInfo,'T');
	char serverAccept;
	char file_length[128];

	// Set up the server address struct
	memset((char*) &server_addr, '\0', sizeof(server_addr));
	port_num = atoi(argv[3]);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_num);

	// only connections on your current machine.
	serverHostInfo = gethostbyname("localhost");
	if (serverHostInfo == NULL) { 
		fprintf(stderr, "CLIENT: ERROR, no such host\n");
		exit(1);
	}
	memcpy((char*) &server_addr.sin_addr.s_addr, (char*) serverHostInfo->h_addr, serverHostInfo->h_length);

	// Create and configure the socket
	sock_file_description = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_file_description < 0){ 
		error("CLIENT: ERROR opening socket");
	}
	
	// Connect to server
	if (connect(sock_file_description, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
	 	error("CLIENT: ERROR connecting");
	}

	send(sock_file_description, &(PROGRAM_NAME[0]), sizeof(char), 0);
	recv(sock_file_description, &serverAccept, sizeof(char), 0);
	if(serverAccept != 'Y'){
		close(sock_file_description);
		err_helper("not an encryption server.");
	}

	//sending data to server.
	send(sock_file_description, &(FileInfo->file_length), sizeof(FileInfo->file_length), 0);//send int of file length.
	send(sock_file_description, key_text,FileInfo->file_length * sizeof(char), 0); // send key text.
	send(sock_file_description, raw_text,FileInfo->file_length * sizeof(char), 0); // send mesage text.

	//recieving data from server.
	recv(sock_file_description,response_text, FileInfo->file_length * sizeof(char), 0); //retrive encrpyted text.
	printf("%s\n", response_text);

	//freeing data.
	clear_encryption_obj(FileInfo);
	free(key_text);
	free(raw_text);
	free(response_text);
	free(FileInfo);

	return 0;
}
