CC = gcc
CFLAGS = -std=c99 -O3 -Wall -Wpedantic
# CFLAGS = -std=c99 -O3 -Wall -Wpedantic -g

SRC = server.c
TARGET = image_tagger

all: $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS)

clean:
	rm -f $(TARGET) *.o
