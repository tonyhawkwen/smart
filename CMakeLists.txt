cmake_minimum_required (VERSION 2.6)

PROJECT (Stone)

INCLUDE(FindPkgConfig)

PKG_CHECK_MODULES(PKGS REQUIRED
				  libev
				  libgflags
				  hiredis
                  openssl
				  )


INCLUDE_DIRECTORIES(${PKGS_INCLUDE_DIRS})


INCLUDE_DIRECTORIES(public
                    core
                    redis
                    http
                    small
                    authorize
                    business)

SET(SRC_LIST public/sockets.cpp
             public/ssl.cpp
             public/md5.cpp
             http/http_parser.cpp
             http/http_message.cpp
             core/loop.cpp
             core/loop_thread.cpp
             redis/redis_proxy.cpp
             redis/redis_process.cpp
             small/tcp_service.cpp
             authorize/auth_service.cpp
             business/customer_service.cpp
             server_small.cpp)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")
ADD_DEFINITIONS(-DELPP_THREAD_SAFE)
ADD_DEFINITIONS(-DELPP_FORCE_USE_STD_THREAD)
ADD_DEFINITIONS(-DELPP_NO_DEFAULT_LOG_FILE)
ADD_DEFINITIONS(-DELPP_STACKTRACE_ON_CRASH)
ADD_DEFINITIONS(-DELPP_HANDLE_SIGABRT)

ADD_EXECUTABLE(smart ${SRC_LIST})
TARGET_LINK_LIBRARIES(smart "${PKGS_LIBRARIES} -lpthread -ldl")
