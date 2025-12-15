cmake_minimum_required(VERSION 3.15)
project(ePSU)

set(CMAKE_BUILD_TYPE "Debug")

FIND_PACKAGE(OpenMP REQUIRED)

add_definitions(-w)

SET(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")

set(OPENSSL_ROOT_DIR /usr/local/openssl)
set(OPENSSL_INCLUDE_DIR /usr/local/openssl/include)
set(OPENSSL_LIBRARY_DIR /usr/local/openssl/lib64)

find_package(volePSI REQUIRED HINTS "/home/byj/桌面/history/skMPSU/ZC/libvolepsi")


include_directories("../Kunlun/pke")
include_directories(JPEQT)

file(GLOB_RECURSE OTHER_SRCS
    JPEQT/*.cpp
)

set(SRCS ${OTHER_SRCS} JPEQT.cpp)

message("Source files: ${SRCS}")

set(CMAKE_BUILD_TYPE Release)

add_executable(JPEQT ${SRCS})
target_include_directories(JPEQT PRIVATE /home/byj/桌面/history/skMPSU/ZC/libvolepsi/include)

target_compile_options(JPEQT PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++17> -lpthread -O3)
target_link_libraries(JPEQT visa::volePSI ${OPENSSL_LIBRARY_DIR}/libssl.a ${OPENSSL_LIBRARY_DIR}/libcrypto.a OpenMP::OpenMP_CXX)
