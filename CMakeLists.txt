cmake_minimum_required(VERSION 3.5)
project(mozquic_example)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-pthread -MP -MD -fno-exceptions -fno-rtti -Wno-format -g")
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(MOZQUIC_SEARCH_DIR /home/jakob/CLionProjects/mozquic)


include_directories(mozquic)

set (
    MOZQUIC_FILES

    mozquic/Ack.cpp
    mozquic/API.cpp
    mozquic/Handshake.cpp
    mozquic/Logging.cpp
    mozquic/Logging.h
    mozquic/MozQuic.cpp
    mozquic/MozQuic.h
    mozquic/NSSHelper.cpp
    mozquic/NSSHelper.h
    mozquic/Packetization.cpp
    mozquic/Packetization.h
    mozquic/Ping.cpp
    mozquic/Sender.cpp
    mozquic/Sender.h
    mozquic/StatelessReset.cpp
    mozquic/Streams.cpp
    mozquic/Streams.h
    mozquic/Timer.cpp
    mozquic/Timer.h
    mozquic/TransportExtension.cpp
    mozquic/TransportExtension.h
)

set(
    MOZQUIC_EXAMPLE_FILES

    main.cpp
    Client.cpp
    Client.h
    Server.cpp
    Server.h
    mozquic_helper.h
    Trigger.cpp
    Trigger.h
)

#### FIND ####
#### MOZQUIC ####
#if(NOT MOZQUIC_ROOT_DIR)
#    set(MOZQUIC_ROOT_DIR ${MOZQUIC_SEARCH_DIR})
#endif()
#find_package(Mozquic)
#if(MOZQUIC_FOUND)
#    include_directories(${MOZQUIC_INCLUDE_DIR})
#else()
#    message(FATAL_ERROR "could not find MOZQUIC")
#endif()

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

add_executable(mozquic_example
        ${MOZQUIC_EXAMPLE_FILES}
        ${MOZQUIC_FILES}
        )
target_link_libraries(
    mozquic_example
    ${NSS_LIBRARIES}
  	${NSPR_LIBRARIES}
    -lstdc++
)

