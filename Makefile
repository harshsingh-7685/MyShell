CC=gcc

TARGET = ./mysh
SRC = ./mysh.c

TEST_PATH = ./myscript.sh

.PHONY: all clean run

all: $(TARGET) run

$(TARGET): $(SRC)
	$(CC) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET) $(TEST_PATH)