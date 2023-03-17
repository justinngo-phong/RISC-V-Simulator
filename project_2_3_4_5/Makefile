SOURCE	:= Main.c Parser.c Registers.c Core.c
CC	:= gcc -std=gnu99 -g -Wall
TARGET	:= RVSim

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)
