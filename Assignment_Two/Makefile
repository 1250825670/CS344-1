CC=gcc
CFLAGS=-g -Wall -std=c99

all:
	${CC} miranale.buildrooms.c -o miranale.builtRooms ${CFLAGS}
	./miranale.builtRooms
	${CC} miranale.adventure.c -o miranale.adventure ${CFLAGS}

clean:
	rm -f miranale.builtRooms
	rm -f miranale.adventure
	rm -rf *.rooms.*
	rm -rf miranale.builtRooms.dSYM
	rm -rf miranale.adventure.dSYM
	rm -f timeOutput.txt