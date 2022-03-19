cmake_minimum_required(VERSION 3.0)

if(CMAKE_CONFIGURATION_TYPES)
   set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo)
   set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
     "Reset the configurations to Debug and Release"
     FORCE)
endif()

if (NOT CMAKE_BUILD_TYPE) 
    set (CMAKE_BUILD_TYPE "Release") 
endif ()

project (animClip) 
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

# Packages
if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif ()

find_package( Maya REQUIRED )

include_directories(${MAYA_INCLUDE_DIR})

set(sources sources/main.cpp 
	sources/saveAnimClipCommand.cpp
	sources/saveAnimClipCommand.h
	sources/loadAnimClipCommand.cpp
	sources/loadAnimClipCommand.h)

add_library(animClip SHARED ${sources})

target_link_libraries( animClip ${MAYA_Foundation_LIBRARY} ${MAYA_OpenMaya_LIBRARY} ${MAYA_OpenMayaAnim_LIBRARY})
set_target_properties( animClip PROPERTIES OUTPUT_NAME animClip CLEAN_DIRECT_OUTPUT 1 SUFFIX ".mll" )

MAYA_SET_PLUGIN_PROPERTIES( animClip )

install(TARGETS animClip RUNTIME DESTINATION .)

