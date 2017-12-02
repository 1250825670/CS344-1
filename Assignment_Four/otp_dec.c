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
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFF_SIZE 256

//NON MACRO GLOBALS
char * PROGRAM_NAME = "DEC_CLIENT";

//Struct for passing file data around
struct file_data_obj {
	int key;
	int text;
	int file_length;
	char * key_filename;
	char * text_filename;
};

// Defining file_data_obj so can omit struct keyword
typedef struct file_data_obj file_data_obj;

// Professor provided error function
void error(const char * msg) { 
	perror(msg); 
	exit(0);
} 

/// NAME: err_helper
/// DESC: My error function that prints program name at beginning.
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

/// NAME: create_encryption_obj
/// DESC: creates an encryption object to store file data.
file_data_obj * create_encryption_obj(char ** argv) {
	file_data_obj* file_ob = malloc(1 * sizeof(file_data_obj));

	// set file names.
	file_ob->text_filename = argv[1];
	file_ob->key_filename = argv[2];

	// open text file to get descriptor.
	file_ob->text = open(argv[1],O_RDONLY);
	if(file_ob->text < 0){
		err_helper("Couldnt open Cipherfile file");
	}
	// open key file to get descriptor.
	file_ob->key = open(argv[2],O_RDONLY);
	if(file_ob->key < 0){
		err_helper("Couldnt open key_text file");
	}

	return file_ob;
}

/**
 * Opens and returns a file's contents
 * 
 * Obj: {struct file_data_obj *} - Pointer to the file_data_obj
 * fd: 
 * 
*/
char * read_file(file_data_obj* obj, char fd) {
	int i, file_description;
	char * fileContent = malloc( obj->file_length * sizeof(char*));
	int Buffer;

	//conditionsal for which file descriptor we are using.
	if(fd == 'K'){
		file_description = obj->key;
	}
	else if(fd == 'T'){
		file_description = obj->text;
	}

	// read content of file and return an address to it in the heap.
	if(read(file_description, fileContent,obj->file_length) < 0) {
		err_helper("Couldnt open text file");
	}
	return fileContent;
}

/// NAME: close_encrypt_file_description
/// DESC: Clean up function for file descriptors.
void close_encrypt_file_description(file_data_obj * obj) {
	//close desciptors.
	close(obj->text);
	close(obj->key);

	//set to invalid num to be overwritten.
	obj->text = -1;
	obj->key = -1;
}

/// NAME: get_encryption_file_obj_file_length
/// DESC: checks if key file is longer than text then sets struct file length.
/**
 * 
*/
int get_encryption_file_obj_file_length(file_data_obj * obj) {
	struct stat key;
	struct stat text;

	if (stat(obj->key_filename, &key) < 0) {
		err_helper("Error getting stats of key file.");
	}

	if (stat(obj->text_filename, &text) < 0) {
		err_helper("Error getting stats of cipher.");
	}

	if (key.st_size - 1 < text.st_size - 1) {
		err_helper("Key file is too short.");
	} else {
		obj->file_length = text.st_size -1;
	}

	return obj->file_length;
}

/**
 * Main routine for the otp_dec file
 * 
 * argc: {Integer} - The number of args provided to the executable
 * argv: {char * Array} - The char * Array representing the args passed to the executable
 * 
 * returns: {Integer} - Returns an integer representing success or failure
*/
int main (int argc, char* argv[]) {
	int sock_file_description;
	int port_num;
	struct sockaddr_in server_addr;
	struct hostent* serverHostInfo;
	char buffer[BUFF_SIZE];
    
	//validate args and prep file_data struct.
	validArgc(argc);
	file_data_obj* file_data = create_encryption_obj(argv);
	get_encryption_file_obj_file_length(file_data);

	// Parsing key and text contents
	char * key_text = read_file(file_data, 'K');
	char * response_text = malloc(file_data->file_length * sizeof(char*));
	char * raw_text= read_file(file_data, 'T');
	char serverAccept;
	char file_length[128];

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
	memcpy((char*)&server_addr.sin_addr.s_addr, (char*) serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	sock_file_description = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (sock_file_description < 0){ 
		error("CLIENT: ERROR opening socket");
	}
	
	// Connect to server
	if (connect(sock_file_description, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0){ // Connect socket to address
	 	error("CLIENT: ERROR connecting");
	}


	// Logging into the server
	send(sock_file_description, &(PROGRAM_NAME[0]), sizeof(char), 0);//send client info D for decrption client.
	recv(sock_file_description, &serverAccept, sizeof(char), 0);// get Y or N for matching server.

	if(serverAccept != 'Y'){
		close(sock_file_description); // if not valid exit.
		err_helper("not an Decryption server.");
	}

	// transmitting file data to the server
	send(sock_file_description, &(file_data->file_length), sizeof(file_data->file_length), 0);
	send(sock_file_description, key_text, file_data->file_length * sizeof(char), 0);
	send(sock_file_description, raw_text, file_data->file_length * sizeof(char), 0);

	// 
	recv(sock_file_description, response_text, file_data->file_length * sizeof(char), 0);//retrive decrypted text.
	printf("%s\n", response_text);

	// Deallocating the memory dynamically allocated
	close_encrypt_file_description(file_data);
	free(key_text);
	free(raw_text);
	free(response_text);
	free(file_data);

	return 0;
}
