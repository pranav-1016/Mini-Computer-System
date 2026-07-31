CC = gcc
CFLAGS = -Wall -g

TARGET = simulator

OBJS = main.o compiler.o processor.o memory.o

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

main.o: main.c compiler.h processor.h memory.h
	$(CC) $(CFLAGS) -c main.c

compiler.o: compiler.c compiler.h
	$(CC) $(CFLAGS) -c compiler.c
memory.o: memory.c memory.h
	$(CC) $(CFLAGS) -c memory.c
processor.o: processor.c processor.h
	$(CC) $(CFLAGS) -c processor.c

