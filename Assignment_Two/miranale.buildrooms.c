/**
 * Name: Alexander Miranda
 * Assignment: Program 2 Adventure Game
 * Due Date: 10/25/2017
 * Course: CS 344
*/

#include <stdlib.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

// Defining configuration constants based on the game specification
#define MAX_ADVENTURE_ROOM_COUNT 7
#define MAX_GAME_ROOMS 10
#define LOWER_ROOM_CONNECTION_COUNT 3
#define UPPER_ROOM_CONNECTION_COUNT 6
#define BUFFER_SIZE 256

/**
 * Room titles that the rooms could be named that are
 * inspired by Seattle neighborhood names
*/
char * ROOM_TITLES[MAX_GAME_ROOMS] = {
    "Fremont",
    "Capitol_Hill",
    "Queen_Anne",
    "Maple_Leaf",
    "Wallingford",
    "Greenwood",
    "Ballard",
    "Belltown",
    "South_Lake_Union",
    "University_District"
};

/**
 * Enum that stores the different room types for the ROOM struct
*/
enum TypeOfRoom{
    START_ROOM,
    MID_ROOM,
    END_ROOM
};

/**
 * ROOM struct that holds the necessary data each room
 * needs to have:
 * number of connections
 * array storing pointers to the connected rooms
 * field designated what type of room it is
 * a name field denoting the room name
*/
struct ROOM
{
    int currentConnectionCount;
    char name[BUFFER_SIZE];
    struct ROOM * connectionArr[UPPER_ROOM_CONNECTION_COUNT];
    enum TypeOfRoom roomType;
};

// Program globals used for the construction of the room dir and room files
struct ROOM roomArray[MAX_ADVENTURE_ROOM_COUNT];
int initializedRooms[MAX_GAME_ROOMS];
char directoryName[BUFFER_SIZE];

/**
 * Checks if the current room have the max allowed room connections
 * 
 * roomIndex: The index in the roomArray that the room is located within
 * 
 * returns: whether the room has the max number of connections or not
*/
int IsConnectionArrFull(int roomIndex)
{
    if(roomArray[roomIndex].currentConnectionCount == UPPER_ROOM_CONNECTION_COUNT){
        return 0;
    }
    else{
        return 1;
    }
}

/**
 * Checks if the two rooms are already connected or if the first room already
 * has the maximum number of allowable connections
 * 
 * roomIndex1: array index of the first room being compared
 * roomIndex2: array index of the second room being compared
 * 
 * returns: whether the rooms are connected or not or if the first room has enough rooms
*/
int AreRoomsConnected(int roomIndex1, int roomIndex2)
{
    //this tells the connect room function not to connect the rooms 
    if(roomArray[roomIndex1].currentConnectionCount == UPPER_ROOM_CONNECTION_COUNT){
        return 1;
    }

    for(int i = 0; i < roomArray[roomIndex1].currentConnectionCount;i++){
        if(roomArray[roomIndex1].connectionArr[i] == NULL ) {
            return 0;
        }
        else if(strcmp(roomArray[roomIndex1].connectionArr[i]->name, roomArray[roomIndex2].name) == 0){
            return 1;
        }
    }
    return 0;
}

/**
 * Helper method that checks if the two rooms are connected or if they are the same room
 * 
 * roomIndex1: Index number for the first room position in the roomArray
 * roomIndex2: Index number for the second room position in the roomArray
 * 
 * returns: whether the rooms are connected or are the same room
*/
int IsConnected(int roomIndex1, int roomIndex2)
{
    // Checking if the two rooms are the same because they would
    // be the same index value
    if (roomIndex1 == roomIndex2) {
        return 1;
    }

    if (AreRoomsConnected(roomIndex1, roomIndex2) == 1) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * Function that connects the room at the passed in index to an available room
 * 
 * roomIndex: the index of the current room being connected
*/
void ConnectRoom(int roomIndex)
{
    int connected = 0;
    int roomOne;
    int roomTwo;

    if (roomArray[roomIndex].currentConnectionCount == UPPER_ROOM_CONNECTION_COUNT) {
        return;
    }

    do {
        roomOne = roomIndex;
        roomTwo = RandomRoomWithinRange(0, MAX_ADVENTURE_ROOM_COUNT);

        if (IsConnected(roomOne, roomTwo) == 0) { // begin connecting the rooms.
            int connectionCount1 = roomArray[roomOne].currentConnectionCount;
            int connectionCount2 = roomArray[roomTwo].currentConnectionCount;

            roomArray[roomOne].connectionArr[connectionCount1] = &roomArray[roomTwo];
            roomArray[roomTwo].connectionArr[connectionCount2] = &roomArray[roomOne];
            roomArray[roomOne].currentConnectionCount++;
            roomArray[roomTwo].currentConnectionCount++;
            connected = 1;
        }
    } while(connected == 0);
}

/**
 * Function that returns a random integer that serves as the index
 * for the room position in the roomArray
 * 
 * returns: integer that represents the room index position in the roomArray
*/
int SelectRandomRoom()
{
    int roomIndex;

    do {
        roomIndex = rand() % MAX_GAME_ROOMS;
    } while (IsConnectionArrFull(roomIndex) == 0);

    return roomIndex;
}

/**
 * Generates a random integer between an upper and lower bound provided as parameters
 * 
 * lowerBound: integer representing the lower limit for the random int generation
 * upperBound: integer representing the upper limit for the random int generation
 * 
 * returns: integer that was randomly determined to serve as the room index
 * for the room selection
*/
int RandomRoomWithinRange(int lowerBound, int upperBound)
{
    int roomIndex;

    do {
        roomIndex = rand() % upperBound + lowerBound; // generates random index between the min and max values
    } while(IsConnectionArrFull(roomIndex) == 0 && roomIndex <= upperBound && roomIndex >= lowerBound); // check if numbers are out of range.

    return roomIndex;
}

/**
 * Method that overwrites the rooms connectionArr with initial NULL values
 * 
 * roomIndex: The index position of the room that will have its connectionArr 
 * initialized to NULL
*/
void initRoomconnectionArr(int roomIndex){
    for(int i = 0; i < UPPER_ROOM_CONNECTION_COUNT; i++){
            roomArray[roomIndex].connectionArr[i] = NULL;
    }
}

/**
 * Generates a directory that will contain the room files using
 * specified permissions
*/
void CreateRoomDir()
{
    char * dirName = "miranale.rooms.";
    int pid = getpid();
    int permissionSetting = 0770;

    memset(directoryName, '\0', sizeof(directoryName));
    sprintf(directoryName, "%s%d", dirName, pid);

    mkdir(directoryName, permissionSetting);
}

/**
 * Function that initializes the rooms array with initial values
*/
void InitializeRoomArray()
{
    // tells me which 7 rooms I picked out of 10
    for(int i = 0; i < MAX_ADVENTURE_ROOM_COUNT; i++) {
        initializedRooms[i] = 0;
    }

    // Iterates through the roomArray and populates it with the rooms
    // for the next game
    for (int i = 0; i < MAX_ADVENTURE_ROOM_COUNT; i++) {
        roomArray[i].currentConnectionCount = 0;
        initRoomconnectionArr(i);
    
        while (1) {
            int randomRoomIndex = SelectRandomRoom();
            if (initializedRooms[randomRoomIndex] == 0) {
                initializedRooms[randomRoomIndex] = 1;
                memset(roomArray[i].name,'\0', sizeof(roomArray[i].name));
                strcpy(roomArray[i].name,ROOM_TITLES[randomRoomIndex]);
                roomArray[i].roomType = MID_ROOM;
                break;
            }
        }
    }

    // Assigning roomType to the starting and ending rooms
    roomArray[0].roomType = START_ROOM;
    roomArray[MAX_ADVENTURE_ROOM_COUNT - 1].roomType = END_ROOM;
}

/**
 * Generates the room graph by connecting relevant rooms
 * in a random fashion
*/
void PopulateRoomArray()
{
    InitializeRoomArray();
    for(int i = 0; i < MAX_ADVENTURE_ROOM_COUNT; i++){
        for(int j = 0; j < LOWER_ROOM_CONNECTION_COUNT; j++){
            ConnectRoom(i);
        }
    }
}

/**
 * Function that will write and output the room files to the current working room directory
*/
void WriteRoomFiles()
{
    FILE * roomFilePtr;
    char dirName[BUFFER_SIZE];

    sprintf(dirName, "./miranale.rooms.%d", getpid());

    // Generating a room directory for the room files
    CreateRoomDir(); 
    
    // prevents errors, checks if the folder exists or directory was changed.
    if(chdir(dirName) != 0){
        printf("DIR NOT CHANGED TO: %s\n", dirName);
        return;
    }

    // Iteration to creating the room files
    for(int i = 0; i < MAX_ADVENTURE_ROOM_COUNT; i++){
        roomFilePtr = fopen(roomArray[i].name,"w");

        fprintf(roomFilePtr,"ROOM NAME: %s\n", roomArray[i].name);
        for(int j = 0;j < roomArray[i].currentConnectionCount;j++){
            fprintf(roomFilePtr,"CONNECTION %d: %s\n", j+1, roomArray[i].connectionArr[j]->name);
        }
        
        if(roomArray[i].roomType == START_ROOM){
            fprintf(roomFilePtr,"ROOM TYPE: %s\n", "START_ROOM");
        }
        else if(roomArray[i].roomType == MID_ROOM){
            fprintf(roomFilePtr,"ROOM TYPE: %s\n", "MID_ROOM");
        }
        else if(roomArray[i].roomType == END_ROOM){
            fprintf(roomFilePtr,"ROOM TYPE: %s\n", "END_ROOM");
        }
        else{
            fprintf(roomFilePtr,"ROOM TYPE: %s\n", "NULL");
        }
        fclose(roomFilePtr);
    }
}

/**
 * Main routine for the buildroom executable
 * seeds the randomizer, fills the room array
 * and writes each room file into a designated directory
 * 
 * returns: integer returning showing successful execution
*/
int main()
{
    srand(time(NULL));
    PopulateRoomArray();
    WriteRoomFiles();

    return 0;
}