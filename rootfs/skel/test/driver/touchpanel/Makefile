# Makefile for alsa test routine

SRC = input-test.c
BIN = input-test

CC = arm-none-linux-gnueabi-gcc

all:
	$(CC) $(SRC) ${LIB} -static -o $(BIN)

clean:
	rm -f $(BIN)
