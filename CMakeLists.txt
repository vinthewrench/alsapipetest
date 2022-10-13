SET (CMAKE_C_COMPILER             "clang")
SET (CMAKE_CXX_COMPILER            "clang++")
set(CMAKE_CXX_FLAGS "-Wall -std=c++17 -O2 -ffast-math -ftree-vectorize ${EXTRA_FLAGS}")


cmake_minimum_required(VERSION 3.0)

if ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
    message( FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt." )
endif()

project(alsapipetest)

find_package(Threads)
find_package(ALSA REQUIRED)

set(SOURCES
	src/main.cpp
	src/ShairportMgr.cpp
	src/dbuf.cpp
)

 
add_executable(alsapipetest ${SOURCES})


set_target_properties(alsapipetest PROPERTIES
				CXX_STANDARD 17
				CXX_EXTENSIONS OFF
				)
 
set_target_properties(alsapipetest PROPERTIES
			OUTPUT_NAME "alsapipetest"
			)

			  
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(
    ${ALSA_INCLUDE_DIRS}
    ${EXTRA_INCLUDES} )

link_directories(
    ${ALSA_INCLUDE_DIRS}
    ${EXTRA_INCLUDES}
    )

target_link_libraries(alsapipetest
	 PRIVATE
	 ${CMAKE_THREAD_LIBS_INIT}
	 ${ALSA_LIBRARIES}
    ${EXTRA_LIBS}
 	 Threads::Threads
 	)

target_include_directories(alsapipetest
	PRIVATE
	src
)

set(CMAKE_BINARY_DIR "bin")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
