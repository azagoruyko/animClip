cmake_minimum_required(VERSION 3.0)

project (animClip) 
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package( Maya REQUIRED )

set(sources sources/main.cpp 
	sources/saveAnimClipCommand.cpp
	sources/saveAnimClipCommand.h
	sources/loadAnimClipCommand.cpp
	sources/loadAnimClipCommand.h)

add_library(animClip SHARED ${sources})

MAYA_PLUGIN( animClip )

