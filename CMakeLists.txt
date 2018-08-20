cmake_minimum_required(VERSION 3.5)
project(mozquic_example)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -MP -MD -fno-exceptions -fno-rtti -Wno-format -g")

# enable asan
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_STATIC_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(MOZQUIC_SEARCH_DIR /home/jakob/CLionProjects/mozquic)

set(
        CLIENT_FILES

        Client.cpp
        Client.h
        mozquic_helper.h
        Trigger.cpp
        Trigger.h
)
set (
        SERVER_FILES

        Server.cpp
        Server.h
        mozquic_helper.h
)

#### FIND ####
#### MOZQUIC ####
if(NOT MOZQUIC_ROOT_DIR)
    set(MOZQUIC_ROOT_DIR ${MOZQUIC_SEARCH_DIR})
endif()
if(NOT MOZQUIC_LIB_ROOT_DIR)
    set(MOZQUIC_LIB_ROOT_DIR ${MOZQUIC_SEARCH_DIR}/cmake_build)
endif()
find_package(Mozquic)
if (MOZQUIC_FOUND)
    include_directories(${MOZQUIC_INCLUDE_DIRS})
    message(STATUS "MOZQUIC_INCLUDE_DIRS: ${MOZQUIC_INCLUDE_DIRS}")
else ()
    message(FATAL_ERROR "Could not find MOZQUIC.")
endif ()

#### NSS ####
find_package(NSS)
if (NSS_FOUND)
    include_directories(${NSS_INCLUDE_DIRS})
    message(STATUS "NSS_INCLUDE_DIRS: ${NSS_INCLUDE_DIRS}")
else ()
    message(FATAL_ERROR "Could not find NSS.")
endif ()

#### NSPR ####
find_package(NSPR)
if(NSPR_FOUND)
    include_directories(${NSPR_INCLUDE_DIRS})
    message(STATUS "NSPR_INCLUDE_DIRS: ${NSPR_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "could not find NSPR.")
endif()

# client
add_executable(
        client
        ${CLIENT_FILES}
)
target_link_libraries(
        client
        ${MOZQUIC_LIBRARIES}
        ${NSS_LIBRARIES}
        ${NSPR_LIBRARIES}
        -lstdc++
)

# server
add_executable(
        server
        ${SERVER_FILES}
        ${MOZQUIC_FILES}
)
target_link_libraries(
        server
        ${MOZQUIC_LIBRARIES}
        ${NSS_LIBRARIES}
        ${NSPR_LIBRARIES}
        -lstdc++
)

