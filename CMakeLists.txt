cmake_minimum_required(VERSION 3.10)
project(Streaming C)

# SQLite library
find_package(SQLite3 REQUIRED)

# server executable file
set(SOURCE_FILES src/server.c src/actions.c src/database.c)
add_executable(server ${SOURCE_FILES})

# client executable file
add_executable(client src/client.c)

# adding SQLite to executable file
target_link_libraries(server SQLite::SQLite3)