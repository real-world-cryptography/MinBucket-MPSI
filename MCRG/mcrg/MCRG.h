#pragma once
#include "Defines.h"
#include <volePSI/Defines.h>
#include <volePSI/config.h>
#include <volePSI/Paxos.h>
#include <volePSI/SimpleIndex.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <cryptoTools/Network/Channel.h>
#include <cryptoTools/Common/CuckooIndex.h>
#include <cryptoTools/Common/Timer.h>
#include <cryptoTools/Common/BitVector.h>
#include <libOTe/Vole/Silent/SilentVoleReceiver.h>
#include <libOTe/Vole/Silent/SilentVoleSender.h>
#include <coproto/Socket/AsioSocket.h>
#include <string> 
#include <fstream>
#include <iostream>
#include <algorithm>


using namespace oc;

constexpr int TEST_NUM =9;
inline u32 numBins;

void MCRG_sender(u32 numElements, std::vector<Socket> &chl_send,std::vector<Socket> &chl_recv, u32 numThreads,u32 tn);
void MCRG_receiver(u32 idx,u32 numElements, Socket &chl_send,Socket &chl_recv, u32 numThreads);








