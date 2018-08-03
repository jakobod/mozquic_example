cmake_minimum_required(VERSION 3.5)
project(mozquic_example)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(MOZQUIC_SEARCH_DIR /home/boss/Desktop/mozilla/mozquic)

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
find_package(Mozquic REQUIRED)
if(MOZQUIC_FOUND)
    include_directories(${MOZQUIC_INCLUDE_DIR})
else()
    message(FATAL_ERROR "could not find MOZQUIC")
endif()
message(mozquic include dir: ${MOZQUIC_INCLUDE_DIR})
message(mozquic library: ${MOZQUIC_LIBRARY})
include_directories(${MOZQUIC_INCLUDE_DIR})

#### NSS ####
find_package(NSS REQUIRED)
if (NSS_FOUND)
    include_directories(${NSS_INCLUDE_DIRS})
else ()
    message(FATAL_ERROR "Could not find NSS.")
endif ()
message(nss libraries: ${NSS_LIBRARIES})
message(nss include dirs: ${NSS_INCLUDE_DIRS})
include_directories(${NSS_INCLUDE_DIRS})
include_directories(${MOZQUIC_SEARCH_DIR}/../nss/lib/ssl/)

add_executable(mozquic_example
        ${MOZQUIC_EXAMPLE_FILES}
        ${MOZQUIC_SEARCH_DIR}/../nss/lib/ssl/ssl.h
)
target_link_libraries(
        mozquic_example
        ${MOZQUIC_LIBRARY}
        ${NSS_LIBRARIES}
)

#link to nss