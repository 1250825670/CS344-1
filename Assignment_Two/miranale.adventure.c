/**
 * Name: Alexander Miranda
 * Assignment: Program 2 Adventure Game
 * Due Date: 10/25/2017
 * Course: CS 344
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <pthread.h>

// Defining constants used throughout the program
#define MAX_ADVENTURE_ROOM_COUNT 7
#define MAX_GAME_ROOMS 10
#define LOWER_ROOM_CONNECTION_COUNT 3
#define UPPER_ROOM_CONNECTION_COUNT 6
#define BUFFER_SIZE 256

/**
 * Enum that stores the different room types for the ROOM struct
*/
enum ROOM_TYPES {
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
    struct ROOM * connectionArr[UPPER_ROOM_CONNECTION_COUNT];
    enum ROOM_TYPES roomType;
    char name[BUFFER_SIZE];
};

// Program globals used for the running of the adventure game
char * timeOutputFileName = "timeOutput.txt";
struct ROOM roomArray[MAX_ADVENTURE_ROOM_COUNT];
pthread_mutex_t timeOutputFilePtr_Mutex;
char dirName[BUFFER_SIZE];

/**
 * Function: CleareddirNameGlobal
 * Helper method that cleans the dirName directory global
 * 
*/
void CleanDirectoryName()
{
    memset(dirName, '\0', sizeof(dirName));
}

/**
 * Method that checks for the most recent rooms directory for
 * the game execution
*/
void PickDirectory()
{
    char * fd = "miranale.rooms.";
    struct dirent * dirPtr;
    struct stat * buffer;
    time_t lastModified;
    char currentDir[BUFFER_SIZE];
    DIR * dir;
    time_t isNew = 0;

    buffer = malloc(sizeof(struct stat));
    dirPtr = malloc(sizeof(struct dirent));

    CleanDirectoryName();
    memset(currentDir, '\0', sizeof(currentDir));
    getcwd(currentDir, sizeof(currentDir));
    dir = opendir(currentDir);

    if (dir != NULL) {
        while (dirPtr = readdir(dir)) {	
            if (strstr(dirPtr->d_name,fd) != NULL){
                stat(dirPtr->d_name, buffer);
                lastModified = buffer->st_mtime;

                if(lastModified > isNew) {
                    isNew = lastModified;
                    strcpy(dirName, dirPtr->d_name);
                }
            }
        }
    }
}

/**
 * Clears out the corresponding room's connection array
 * 
 * roomIndex: index in the roomArray for the room for which
 * the connection array will be cleared
*/
void initRoomconnectionArr(int roomIndex){
    for(int i = 0; i < UPPER_ROOM_CONNECTION_COUNT; i++){
            roomArray[roomIndex].connectionArr[i] = NULL;
    }
}

/**
 * Clears out the roomArray
*/
void InitializeRoomArray() {
    for(int i = 0; i < MAX_ADVENTURE_ROOM_COUNT; i++){
        memset(roomArray[i].name, '\0', sizeof(roomArray[i].name));
        roomArray[i].currentConnectionCount = 0;
        initRoomconnectionArr(i);
    }
}

/**
 * Fills the name in for each file
*/
void PopulateRoomArray()
{
    int fileNum = 0;
    struct dirent * ent;
    DIR * directoryPtr;

    InitializeRoomArray();

    if ((directoryPtr = opendir(dirName)) != NULL) {
        while ((ent = readdir (directoryPtr)) != NULL) {
            if (strlen(ent->d_name) > 2) {
                strcpy(roomArray[fileNum].name, ent->d_name);
                fileNum++;
            }
        }
    }
}

/**
 * Helper method that updates the key and value of a given file buffer
*/
void UpdateKeyAndValue(char * keyStr, char * valueStr)
{
    int EOLpos = 0;

    strtok(keyStr, ":");
    strcpy(valueStr, strtok(NULL, ""));
    valueStr[strlen(valueStr) - 1] = '\0';
    keyStr[strlen(keyStr) - 1] = '\0';

    for(int i = 0; i < strlen(valueStr); i++) {
        valueStr[i] = valueStr[i + 1];
    }
}

/**
 * Locates a room's index in the roomArray
 * 
 * roomTitle: The title of the room being searched for
 * 
 * returns: The index of the room with the given roomTitle
*/
int LocateRoomIndexByName(char * roomTitle)
{
    int roomIndex = -1;

    for(int i = 0;i < MAX_ADVENTURE_ROOM_COUNT; i++ ){
        if (strcmp(roomArray[i].name, roomTitle) == 0 ) {
            return i;
        }
    }
    return roomIndex; // return -1 if nothing was found.
}

/**
 * Connects the first room to the second room but does not do vice versa
 * with a single execution
 * 
 * roomOneIndex: room one index
 * roomTwoIndex: room two index
*/
void ConnectRooms(int roomOneIndex, int roomTwoIndex)
{
    int totalConnections1 = roomArray[roomOneIndex].currentConnectionCount;

    roomArray[roomOneIndex].connectionArr[totalConnections1] = &roomArray[roomTwoIndex];
    roomArray[roomOneIndex].currentConnectionCount++;
}

/**
 * Construct the room files and write them to the most recent rooms directory
*/
void GenerateRooms()
{
    char fileNameBuffer[BUFFER_SIZE];
    char fileContentBuffer[BUFFER_SIZE];

    FILE * roomFilePtr;

    PopulateRoomArray(); // fill struct with file names
    chdir(dirName); // change to the directory containing all the files.

    //dont need to check if file exists since we grabed it eariler
    for(int i = 0;i < MAX_ADVENTURE_ROOM_COUNT;i++){
        roomFilePtr = fopen(roomArray[i].name,"r");//OPEN FILE

        if(roomFilePtr == NULL) { // check if file was opened
            printf("%s file was not accessed\n",roomArray[i].name);
            return;
        }

        memset(fileNameBuffer, '\0', sizeof(fileNameBuffer));
        memset(fileContentBuffer, '\0', sizeof(fileContentBuffer));

        // get each line from the file.
        while(fgets(fileNameBuffer, sizeof(fileNameBuffer),roomFilePtr) != NULL){

            //get the label and value from the line.
            UpdateKeyAndValue(fileNameBuffer, fileContentBuffer);
            if(strcmp(fileNameBuffer, "ROOM TYP") == 0) {
                if(strcmp(fileContentBuffer, "START_ROOM") == 0) {
                    roomArray[i].roomType = START_ROOM;
                }
                else if(strcmp(fileContentBuffer, "END_ROOM") == 0) {
                    roomArray[i].roomType = END_ROOM;
                }
                else{
                    roomArray[i].roomType = MID_ROOM;
                }
            }
            else if(strcmp(fileNameBuffer,"CONNECTION ") == 0) {
                int conncroomIndex = LocateRoomIndexByName(fileContentBuffer);
                ConnectRooms(i, conncroomIndex);
            }
        }
        // Close the open roomFile
        fclose(roomFilePtr);
    }
    chdir("..");
}

/**
 * Locates the index of the starting room
 * 
 * returns: the index of the starting room, will return -1 if it was not found
*/
int LocateStartRoomPos(){
    for(int i =0; i < MAX_ADVENTURE_ROOM_COUNT; i++){ // for each room
        if(roomArray[i].roomType == START_ROOM){ 
            return i;
        }
    }
    return -1;
}

/**
 * Generates a text file that has the current user time
 * 
 * returns: a null pointer
*/
void * GenerateCurrentTimeFilePtr()
{
    char timeDisplayString[BUFFER_SIZE];
    time_t currentTime;
    struct tm * timeData;
    FILE * timeOutputFilePtr;

    memset(timeDisplayString, '\0', sizeof(timeDisplayString));

    time(&currentTime);
    timeData = localtime(&currentTime);
    strftime(timeDisplayString, BUFFER_SIZE, "%I:%M%P %A, %B %d, %Y", timeData);

    timeOutputFilePtr = fopen(timeOutputFileName, "w");
    fprintf(timeOutputFilePtr,"%s\n",timeDisplayString);
    fclose(timeOutputFilePtr);

    return NULL;
}

/**
 * Helper method that reads the time output file to 
 * display the current time to the user
*/
void PrintCurrentTime()
{
    char charBuffer[BUFFER_SIZE];
    FILE * timeOutputFilePtr;

    memset(charBuffer, '\0', sizeof(charBuffer));

    timeOutputFilePtr = fopen(timeOutputFileName, "r");
    if(timeOutputFilePtr == NULL) {
        printf("%s could not be accessed\n", timeOutputFileName);
        return;
    }

    while(fgets(charBuffer, BUFFER_SIZE, timeOutputFilePtr) != NULL) {
        printf("%s\n", charBuffer);
    }
    fclose(timeOutputFilePtr);
}

/**
 * Creates a new thread so that the write to the current time can occur
 * 
 * returns: an int indicating whether the thread creation succeeded
*/
int CreateNewThread()
{
    pthread_t WritetimeOutputFilePtr_Thread;
    pthread_mutex_lock(&timeOutputFilePtr_Mutex);

    if (pthread_create(&WritetimeOutputFilePtr_Thread, NULL, GenerateCurrentTimeFilePtr, NULL) != 0) {
        printf("Error from thread!");
        return 0;
    }

    pthread_mutex_unlock(&timeOutputFilePtr_Mutex);
    pthread_join(WritetimeOutputFilePtr_Thread, NULL);

    return 1;
}

/**
 * Method that outputs the path the user took in the game
*/
void printStepPath(int * pathTaken, int totalSteps)
{
    for (int i = 0; i < totalSteps + 1; i++) {
        printf("%s\n", roomArray[pathTaken[i]].name);
    }
}

/**
 * Primary routine that runs the game process
*/
void GameStart()
{
    int currStep = 0;
    int stepRecord[1028];
    struct ROOM currRoom;
    int roomFound = 0;
    int currroomIndex;
    char userInput[BUFFER_SIZE];
    // Declare the iterator for the loops below
    int i;

    stepRecord[currStep] = LocateStartRoomPos();

    do {
        currroomIndex = stepRecord[currStep];
        currRoom = roomArray[currroomIndex];

        printf("CURRENT LOCATION: %s\n", currRoom.name);

        printf("POSSIBLE CONNECTIONS:");
        for (i = 0; i < currRoom.currentConnectionCount - 1; i++) {
            printf(" %s,", currRoom.connectionArr[i]->name);
        }
        printf(" %s.\n", currRoom.connectionArr[i]->name);

        memset(userInput,'\0', sizeof(userInput));
        printf("WHERE TO? >");
        scanf("%255s", userInput);
        printf("\n");

        roomFound = 0;

        for (i = 0; i < currRoom.currentConnectionCount; i++) {
            if(strcmp(userInput, currRoom.connectionArr[i]->name) == 0) {
                ++currStep;
                stepRecord[currStep] = LocateRoomIndexByName(userInput);
                currroomIndex = stepRecord[currStep];
                currRoom = roomArray[currroomIndex];
                roomFound = 1;
                if(currRoom.roomType == END_ROOM) {
                    printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
                    printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n",currStep + 1);
                    printStepPath(stepRecord, currStep);
                    return;
                }
            }
        }

        if(strcmp(userInput,"time") == 0 && roomFound == 0) {
            if (CreateNewThread() == 1) {
                PrintCurrentTime();
            }
        } else if (roomFound == 0) {
            // Error output to the user on invalid input
            printf("HUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
        }
    } while(1);
}

/**
 * Main execution routine for the adventure program
 * 
 * returns: an int indicating successful execution
*/
int main()
{
    PickDirectory();
    GenerateRooms();
    GameStart();

    return 0;
}