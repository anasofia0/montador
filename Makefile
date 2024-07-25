# Makefile for the montador and ligador programs
CC = g++
CFLAGS = -Wall -Wextra -g -std=c++14


SRC_FILES = montador.cpp ligador.cpp
EXECUTABLES = montador ligador


all: $(EXECUTABLES)


montador: montador.cpp
	$(CC) $(CFLAGS) -o montador montador.cpp

ligador: ligador.cpp
	$(CC) $(CFLAGS) -o ligador ligador.cpp


clean:
	rm -f $(EXECUTABLES) *.o


.PHONY: all clean
