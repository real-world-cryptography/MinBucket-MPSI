cmake_minimum_required(VERSION 3.15)
project(pECRGnECRGOTP)


FIND_PACKAGE(OpenMP REQUIRED)


add_definitions(-w)


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

include_directories(mcrg)

file(GLOB_RECURSE SRCS
    mcrg_diff_large/*.cpp
)


set(CMAKE_BUILD_TYPE Release)


add_executable(unbalanced mcrg_diff_large/unbalanced_MCRG.cpp ${SRCS})
target_compile_options(unbalanced PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++17> -lpthread -O3)
target_link_libraries(unbalanced  visa::volePSI ${OPENSSL_LIBRARIES} OpenMP::OpenMP_CXX)












