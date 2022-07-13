cmake_minimum_required(VERSION 3.10)

add_executable(test test.c)
add_test(NAME test COMMAND test)