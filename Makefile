CC=gcc
FLAGS=-std=c11 -Werror -Wall -Wextra
TARGET_PATH=target

all: test

test:
	[ -d $(TARGET_PATH) ] || mkdir -p $(TARGET_PATH)
	$(CC) $(FLAGS) test.c -o $(TARGET_PATH)/test
	$(TARGET_PATH)/test

clean:
	rm $(TARGET)