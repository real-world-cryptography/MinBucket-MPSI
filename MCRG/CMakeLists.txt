cmake_minimum_required(VERSION 3.15)
project(TBB2)


FIND_PACKAGE(OpenMP REQUIRED)

add_definitions(-w)

# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fpic -Wno-deprecated-declarations -Wno-unused-result -Xpreprocessor -O0")

SET(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")

set(OPENSSL_ROOT_DIR /usr/local/openssl)
set(OPENSSL_CRYPTO_LIBRARY /usr/local/openssl/lib64/libcrypto.a)
set(OPENSSL_SSL_LIBRARY /usr/local/openssl/lib64/libssl.a)

find_package(OpenSSL REQUIRED)

include_directories(${OPENSSL_INCLUDE_DIR})
link_directories(${OPENSSL_LIBRARIES})

message(STATUS "OpenSSL include directory:"${OPENSSL_INCLUDE_DIR})
message(STATUS "OpenSSL libraries:"${OPENSSL_LIBRARIES})

find_package(volePSI REQUIRED HINTS "/home/byj/桌面/history/skMPSU/ZC/libvolepsi")

# include_directories(pnMCRG)
# include_directories(test)

file(GLOB_RECURSE SRCS
    mcrg/*.cpp
)


set(CMAKE_BUILD_TYPE Release)

#set(CMAKE_BUILD_TYPE Debug)

add_executable(balanced ${SRCS})
target_compile_options(balanced PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++17> -lpthread -O3)
target_link_libraries(balanced visa::volePSI ${OPENSSL_LIBRARIES} OpenMP::OpenMP_CXX)













