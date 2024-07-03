cmake_minimum_required(VERSION 3.10)

project(IRCeyGUI C)
enable_language(C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${GTK3_INCLUDE_DIRS})

add_executable(IRCeyGUI src/main.c
                        src/gui.h
                        src/gui.c
                        src/callbacks.h
                        src/callbacks.c
                        src/shared.h
                        src/irc.h
                        src/irc.c
                        src/util.h
                        src/util.c)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY build/)

target_link_libraries(IRCeyGUI ${GTK3_LIBRARIES})
#target_link_libraries(IRCeyGUI "-fsanitize=address -fsanitize=leak -fsanitize=undefined")