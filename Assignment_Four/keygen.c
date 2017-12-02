/**
 * Name: Alexander Miranda
 * Due Date: December 1st, 2017
 * Assignment: OTP (One Time Pad)
 * 
*/

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>

#define MIN_LENGTH 65
#define MAX_LENGTH 90

/**
 * Method that generates the encryption key
 * 
 * length: {Integer} - The number of random chars the key will have
 * 
 * returns: {Char *} - The char array representing the generated key
*/
char * generate_key (int length) {
    // Dynamically allocate the memory necessary for the key
    char * key = malloc(sizeof(char) * length);
    int i;

    for(i = 0; i < length; i++) {
        if( (rand() % (10) + 1) == 3 || (rand() % (10) + 1) == 6){
            key[i] = ' ';
        } else{
            key[i] = (rand() % (MAX_LENGTH + 1 - MIN_LENGTH) + MIN_LENGTH);
        }
    }
    // Null terminate the generated key
    key[i] = '\0';

    return key;
}

/**
 * Main executable for the keygen program
 * 
 * argc: {Integer} - The number of arguments being passed to the executable
 * argv: {Char * Array} - The string representation of the arguments passed in
 * 
 * returns: {Integer} - Returns integer denoting successful execution
*/
int main (int argc, char * argv[]) {
    int key_len;
    char * encryption_key;

    if (argc < 2) {
        fprintf(stderr, "keygen error: you must specify the key length to be generated\n\tUsage: keygen <length>\n");
        exit(1);
    }

    srand(time(NULL));
    key_len = atoi(argv[1]) + 1;

    encryption_key = generate_key(key_len);
    printf("%s", encryption_key);

    free(encryption_key);

    return 0;
}