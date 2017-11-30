/**
 * Name: Alexander Miranda
 * Due Date: December 1st, 2017
 * Course: CS 344 - Operating Systems
 * Assignment: OTP (One Time Pad)
*/

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

const char INSTRUCTIONS[] = "Instructions: %s <number>\nnumber is the byte size of the key to be generated\n";

int main(int argc, char * argv[]) {
    srand(time(0));

    int key_len;
    int i;
    int randomChar;

    if (argc != 2) {
        fprintf(stderr, INSTRUCTIONS, argv[0]);
        exit(1);
    }

    key_len = atoi(argv[1]);

    for (i = 0; i < key_len; i++) {
        randomChar = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[rand() % 27];

        if (randomChar == 'A' + 26) {
            randomChar = ' ';
        }

        fprintf(stdout, "%c", randomChar);
    }

    fprintf(stdout, "\n");

    return 0;
}