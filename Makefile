CFLAGS=-g -Wall -Werror -Wunused-variable -fsanitize=address -std=c99 -pthread
LFLAGS=-lm -Wno-unused-variable
OBJ = ttts.c
TARGET = server

all: $(TARGET)

$(TARGET): $(OBJ)
	gcc $(CFLAGS) -o $(TARGET) $(OBJ) $(LFLAGS)
clean:
	rm -f *.o $(OBJ) %(TARGET) *~



