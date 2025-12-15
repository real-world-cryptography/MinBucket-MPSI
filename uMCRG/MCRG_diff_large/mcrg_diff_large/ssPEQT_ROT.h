# pragma once
#include "define.h"
#include "Circuit.h"
#include <cryptoTools/Crypto/PRNG.h>
#include <volePSI/GMW/Gmw.h>
#include <cryptoTools/Common/Timer.h>
#include <cryptoTools/Common/BitVector.h>
#include "cryptoTools/Common/CLP.h"
#include <libOTe/Vole/Silent/SilentVoleReceiver.h>
#include <libOTe/Vole/Silent/SilentVoleSender.h>
#include <libOTe/Base/BaseOT.h>
#include <libOTe/TwoChooseOne/SoftSpokenOT/SoftSpokenShOtExt.h>
#include <coproto/Socket/AsioSocket.h>
#include <volePSI/config.h>
#include <volePSI/Defines.h>
#include <cryptoTools/Network/Channel.h>
#include <string> 
#include <fstream>
#include <iostream>
#include <istream>

#include <algorithm>



using namespace oc;

constexpr uint64_t fieldBits = 5;

void ssPEQT_ROT_sender(u32 numThreads,u32 tn);
void ssPEQT_ROT_receiver(int idx,u32 numThreads);
