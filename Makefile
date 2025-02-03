CC := gcc
CFLAGS := -std=gnu17 -Wall -Wextra -g -O0
LDFLAGS := -lraylib
APP_NAME := gol

SRCS := gol.c
OBJS = $(patsubst %.c, %.o, $(SRCS))

all: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $(APP_NAME)

run: all
	./$(APP_NAME)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(APP_NAME) *.o

.PHONY: all run clean
