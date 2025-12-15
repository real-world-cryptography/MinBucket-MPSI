#include"elgamal.h"
#include"communication.h"
#include"readFiles.h"
#include <string> 
#include <fstream>
#include <iostream>
#include <istream>
#include <algorithm>



constexpr u16 PORT = 1212;
constexpr size_t MSG_LEN=32;
constexpr size_t TRADEOFF_NUM=7;
inline std::random_device rd2;
inline std::mt19937 global_built_in_prg2(rd2());
