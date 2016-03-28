cmake_minimum_required (VERSION 2.6)

PROJECT (Stone)

INCLUDE(FindPkgConfig)

PKG_CHECK_MODULES(PKGS REQUIRED
				  libev
				  libgflags
#				  hiredis
				  )


INCLUDE_DIRECTORIES(${PKGS_INCLUDE_DIRS})


INCLUDE_DIRECTORIES(public
                    core
                    service)


SET(SRC_LIST public/sockets.cpp
             core/loop.cpp
             core/service.cpp
             service/tcp_reader.cpp
             service/tcp_service.cpp
             server.cpp)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -pthread")
ADD_DEFINITIONS(-DELPP_THREAD_SAFE)
ADD_DEFINITIONS(-DELPP_FORCE_USE_STD_THREAD)
ADD_DEFINITIONS(-DELPP_NO_DEFAULT_LOG_FILE)
ADD_DEFINITIONS(-DELPP_STACKTRACE_ON_CRASH)
ADD_DEFINITIONS(-DELPP_HANDLE_SIGABRT)

ADD_EXECUTABLE(smart ${SRC_LIST})
TARGET_LINK_LIBRARIES(smart ${PKGS_LIBRARIES})
