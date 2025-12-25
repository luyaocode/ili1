include ../../Makefile.inc

SRC = test_lock_free_queue.cpp
TARGET = test_lock_free_queue
CC = g++
CFLAGS = -std=c++17 -Wall -fopenmp -O1 -pthread -I../../

all: $(TARGET)
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) -L../../ -lijk -Wl,-rpath=../../
clean: 
	rm -rf $(TARGET)
