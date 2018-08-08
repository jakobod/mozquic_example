cmake_minimum_required(VERSION 3.5)
project(mozquic_example)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(MOZQUIC_SEARCH_DIR /home/jakob/Desktop/mozilla/mozquic)

set(
        MOZQUIC_EXAMPLE_FILES

        main.cpp
        Client.cpp
        Client.h
        Server.cpp
        Server.h
)

#### FIND ####
#### MOZQUIC ####
if(NOT MOZQUIC_ROOT_DIR)
    set(MOZQUIC_ROOT_DIR ${MOZQUIC_SEARCH_DIR})
endif()
find_package(Mozquic)
if(MOZQUIC_FOUND)
    include_directories(${MOZQUIC_INCLUDE_DIR})
else()
    message(FATAL_ERROR "could not find MOZQUIC")
endif()

#### NSS ####
find_package(NSS)
if (NSS_FOUND)
    include_directories(${NSS_INCLUDE_DIRS})
else ()
    message(FATAL_ERROR "Could not find NSS.")
endif ()

#### NSPR ####
find_package(NSPR)
if(NSPR_FOUND)
	include_directories(${NSPR_INCLUDE_DIRS})
else()
	message(FATAL_ERROR "could not find NSPR.")
endif()

add_executable(mozquic_example
        ${MOZQUIC_EXAMPLE_FILES}
)
target_link_libraries(
        mozquic_example
        ${MOZQUIC_LIBRARY}
        ${NSS_LIBRARIES}
	${NSPR_LIBRARIES}
)

