CC=gcc
CXX=g++
C_FLAGS=-std=c11 -Werror -Wall -Wextra
CXX_FLAGS=-std=c++11 -Werror -Wall -Wextra
TARGET_PATH=target

setup:
	[ -d $(TARGET_PATH) ] || mkdir -p $(TARGET_PATH)

all: setup

test: test_c test_cpp
	@echo "All tests succeeded!"

test_c: setup
	$(CC) $(CFLAGS) test.c -o $(TARGET_PATH)/test
	@$(TARGET_PATH)/test

test_cpp: setup
	$(CXX) $(FLAGS) test.c -o $(TARGET_PATH)/test_cpp
	@$(TARGET_PATH)/test_cpp

clean:
	rm $(TARGET_PATH)/*