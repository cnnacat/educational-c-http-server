cmake_minimum_required(VERSION 3.10)
project(http-server)

# Set the source files of the program
# into one variable, called SOURCE_FILES.

set(SOURCE_FILES
	server/server.c
)


# Sets the options for the compiler to be paranoid
# during compilation if the compiler supports such options.

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
	# add_compile_options(-Wall -Werror -pedantic)
endif()


add_executable(http-server ${SOURCE_FILES})


# Window's winsock2.h requires the program to be linked against
# the Ws2_32.lib library.
#
# https://learn.microsoft.com/en-us/windows/win32/winsock/creating-a-basic-winsock-application

if (WIN32)
	target_link_libraries(http-server Ws2_32.lib)
endif()
