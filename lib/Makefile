include Makefile.inc

SRC = libijk.h libijk.cpp common/lock_free_queue.h
TARGET = libijk.so
CC = g++
CFLAGS = -std=c++11 -Wall -fopenmp -O1 -fPIC -shared -Icommon

all: $(TARGET)
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)
clean: 
	rm -rf $(TARGET)
