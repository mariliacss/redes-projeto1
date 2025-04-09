cmake_minimum_required(VERSION 3.10)
project(Streaming C)

# SQLite library
find_package(SQLite3 REQUIRED)

# server executable file
add_executable(server src/server.c)

# client executable file
add_executable(client src/client.c)

# adding SQLite to executable file
target_link_libraries(server SQLite::SQLite3)