SRC = a.cpp b.h b.cpp d.h
TARGET = a
CC = g++
CFLAGS = -std=c++17 -Wall -O3 -pthread

all: $(TARGET)
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)
clean: 
	rm -rf $(TARGET)
