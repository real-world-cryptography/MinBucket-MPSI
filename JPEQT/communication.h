#pragma once
#include "../Kunlun/crypto/ec_point.hpp"
#include"../Kunlun/pke/exponential_elgamal.hpp"
#include <coproto/Socket/AsioSocket.h>
#include <volePSI/config.h>
#include <volePSI/Defines.h>
#include <cryptoTools/Network/Channel.h>

#include <cryptoTools/Crypto/PRNG.h>
#include <cryptoTools/Common/Timer.h>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/CLP.h>
#include <coproto/coproto.h>
#include <coproto/Socket/AsioSocket.h>




using namespace oc;
using u8 = oc::u8;
using u16 = oc::u16;
using u32 = oc::u32;
using u64 = oc::u64;

using CLP = oc::CLP;
using Proto = coproto::task<void>;

using PRNG = oc::PRNG;
using BitVector = oc::BitVector;
using Timer = oc::Timer;
//using oc::block = oc::oc::block;
using Socket = coproto::Socket;



void sendECPoint(Socket &chl, ECPoint &A)
{
    int thread_num = omp_get_thread_num();    
    std::vector<u8> buffer(POINT_COMPRESSED_BYTE_LEN);
    EC_POINT_point2oct(group, A.point_ptr, POINT_CONVERSION_COMPRESSED, buffer.data(), POINT_COMPRESSED_BYTE_LEN, bn_ctx[thread_num]);
    coproto::sync_wait(chl.send(buffer));

}

void recvECPoint(Socket &chl, ECPoint &A)
{
    int thread_num = omp_get_thread_num();    
    std::vector<u8> buffer(POINT_COMPRESSED_BYTE_LEN);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    coproto::sync_wait(chl.recv(buffer));
    EC_POINT_oct2point(group, A.point_ptr, buffer.data(), POINT_COMPRESSED_BYTE_LEN, bn_ctx[thread_num]);

}

void sendCTs(Socket &chl, std::vector<ExponentialElGamal::CT> &VecCT)
{

    u32 num = VecCT.size();
    std::vector<u8> buffer(POINT_COMPRESSED_BYTE_LEN * 2 * num);
    
    int thread_num = omp_get_thread_num();    
    for(u32 i = 0; i < num; ++i)
    {
    	EC_POINT_point2oct(group, VecCT[i].X.point_ptr, POINT_CONVERSION_COMPRESSED, buffer.data()+ 2*i*POINT_COMPRESSED_BYTE_LEN, POINT_COMPRESSED_BYTE_LEN, bn_ctx[thread_num]);
    	EC_POINT_point2oct(group, VecCT[i].Y.point_ptr, POINT_CONVERSION_COMPRESSED, buffer.data()+ (2*i + 1)*POINT_COMPRESSED_BYTE_LEN, POINT_COMPRESSED_BYTE_LEN, bn_ctx[thread_num]);
    	        
    }
    coproto::sync_wait(chl.send(buffer));

}

void recvCTs(Socket &chl, std::vector<ExponentialElGamal::CT> &VecCT)
{
    u32 num = VecCT.size();
    std::vector<u8> buffer(POINT_COMPRESSED_BYTE_LEN * 2 * num);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    coproto::sync_wait(chl.recv(buffer));
    
    int thread_num = omp_get_thread_num();
    
    for(u32 i = 0; i < num; ++i)
    {
    	EC_POINT_oct2point(group, VecCT[i].X.point_ptr, buffer.data() + 2*i*POINT_COMPRESSED_BYTE_LEN , POINT_COMPRESSED_BYTE_LEN, bn_ctx[thread_num]); 
    	EC_POINT_oct2point(group, VecCT[i].Y.point_ptr, buffer.data() + (2*i + 1)*POINT_COMPRESSED_BYTE_LEN , POINT_COMPRESSED_BYTE_LEN, bn_ctx[thread_num]);    	        
    }    
}
