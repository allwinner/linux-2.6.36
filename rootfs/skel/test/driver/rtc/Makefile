# Makefile for alsa test routine

SRC = test_rtc.c
BIN = test_rtc

CC = arm-none-linux-gnueabi-gcc

all:
	$(CC) $(SRC) ${LIB} -static -o $(BIN)

clean:
	rm -f $(BIN)

