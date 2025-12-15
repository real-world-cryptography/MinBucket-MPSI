#pragma once
#include"communication.h"
#include <string> 
#include <fstream>
#include <iostream>
#include <istream>
#include <algorithm>


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
using Socket = coproto::Socket;




inline u32 numBins;
inline std::vector<std::vector<oc::block>> apsu_mcrg;
inline std::vector<std::vector<BigInt>> e ;
inline std::vector<BigInt> apsu_d ;
inline std::vector<BigInt> epsu_d ;


// for P1 to get uMCRG and MCRG result
void read_mcrg_e(u32 tn,u32 bn)
{
    apsu_mcrg.resize(tn-1);
    // first read uMCRG result
    if(bn!=0)
    {
        for (int i = 0; i < bn; i++)
        {
            std::ifstream mcrgFile;
            std::string filePath = "../../uMCRG/MCRG_diff_large/mcrg_result/sender_mcrg_"+std::to_string(i);
            if (!filePath.empty()){
                mcrgFile.open(filePath, std::ios::binary | std::ios::in);
                if (!mcrgFile.is_open()){
                    std::cerr << "Error: Failed to open file: " << filePath << std::endl;
                    return;
                }
        
            }
            mcrgFile.read((char*)(&numBins), sizeof(uint64_t));
            apsu_mcrg[i].resize(numBins);
            mcrgFile.read((char*)apsu_mcrg[i].data(), sizeof(oc::block) * apsu_mcrg[i].size());
            mcrgFile.close();
        }
    }
    
    //then read MCRG result
    for (int i = bn; i < tn-1; i++)
    {
        std::ifstream mcrgFile;
        std::string filePath = "../../MCRG/mcrg/result/sender_result_"+std::to_string(i-bn);
        if (!filePath.empty()){
            mcrgFile.open(filePath, std::ios::binary | std::ios::in);
            if (!mcrgFile.is_open()){
                std::cout << "Error opening file " << filePath << std::endl; 
                return;
            }
        }
        mcrgFile.read((char*)(&numBins), sizeof(uint64_t));
        apsu_mcrg[i].resize(numBins);
        mcrgFile.read((char*)apsu_mcrg[i].data(), sizeof(oc::block) * apsu_mcrg[i].size());
        mcrgFile.close();
    }

    // Convert the element to ELGamal valid
    e.resize(tn-1,std::vector<BigInt>(numBins));
    for (int i = 0; i < tn-1; i++)
    {
        for (int j = 0; j < numBins; j++)
        {
            e[i][j].FromByteString(reinterpret_cast<const unsigned char*>(&apsu_mcrg[i][j]), sizeof(oc::block));
        }
    }
}


// for big-set participants to read uMCRG result
void read_amcrg_d(int sn)
{
    std::ifstream mcrgFile;
    std::string filePath = "../../uMCRG/MCRG_diff_large/mcrg_result/receiver_mcrg_"+std::to_string(sn);
    if (!filePath.empty()){
        mcrgFile.open(filePath, std::ios::binary | std::ios::in);
        if (!mcrgFile.is_open()){
            std::cout<<"Error opening file " << filePath << std::endl; 
            return;
        }
    }
    mcrgFile.read((char*)(&numBins), sizeof(uint64_t));
    std::vector<oc::block>  apsu_mcrg(numBins );
    apsu_d.resize(numBins);
    mcrgFile.read((char*)apsu_mcrg.data(), sizeof(oc::block) * apsu_mcrg.size());
    mcrgFile.close();
    
    for (int i = 0; i < apsu_d.size(); i++)
    {
        apsu_d[i].FromByteString(reinterpret_cast<const unsigned char*>(&apsu_mcrg[i]), sizeof(oc::block));
    }

}

// for big-set participants to read uMCRG result
void read_emcrg_d(int sn)
{
    std::vector<oc::block> epsu_mcrg;
    std::ifstream mcrgFile;
    std::string filePath = "../../MCRG/mcrg/result/receiver_result_"+std::to_string(sn);
    if (!filePath.empty()){
        mcrgFile.open(filePath, std::ios::binary | std::ios::in);
        if (!mcrgFile.is_open()){
            std::cout << "Error opening file " << filePath << std::endl; 
            return;
        }
    }
    mcrgFile.read((char*)(&numBins), sizeof(uint64_t));
    epsu_mcrg.resize(numBins);
    mcrgFile.read((char*)epsu_mcrg.data(), sizeof(oc::block) * epsu_mcrg.size());
    mcrgFile.close();
    epsu_d.resize(numBins);

    for (int i = 0 ; i < epsu_d.size() ; i++) {
        epsu_d[i].FromByteString(reinterpret_cast<const unsigned char*>(&epsu_mcrg[i]), sizeof(oc::block)); 
    }
}

// Write the intersection element to the file
void write_intersection(std::vector<int> index){
    std::string filePath = "../../uMCRG/MCRG/mcrg_result/mapping";
    std::ifstream readItem;
    if (!filePath.empty()){
        readItem.open(filePath, std::ios::binary | std::ios::in);
        if (!readItem.is_open()){
            std::cerr << "Error: Failed to open file: " << filePath << std::endl;
            return;
        }
            
    }
    uint64_t len;
    std::vector<int> index_co;
    readItem.read((char*)(&len), sizeof(uint64_t));   
    index_co.resize(len);
    readItem.read((char*)index_co.data(), sizeof(int)*(index_co.size()));
    readItem.close();

    filePath = "../../uMCRG/MCRG/mcrg_result/orig_item";
    std::ifstream infile(filePath);
    if (!infile.is_open()) {
        std::cerr << "Error: Unable to open file " << filePath << std::endl;
    }

    std::string s;
    std::getline(infile, s);
    int size = atoi(s.c_str());

    std::vector<std::string> temp_items;
    temp_items.reserve(size); 

    std::string line;
    while (std::getline(infile, line)) {
        temp_items.push_back(line); 
    }

    infile.close();
    const std::vector<std::string> orig_items(temp_items.begin(), temp_items.end());
    
    std::ofstream outFile("../../intersection.txt");
    if (!outFile.is_open()) {
        std::cerr << "fail to open file!" << std::endl;
        return;
    }
    
    std::streambuf* coutBuf = std::cout.rdbuf();
    std::cout.rdbuf(outFile.rdbuf()); 
    
    for (int i = 0; i < index.size(); i++)
    {
        std::cout<<orig_items[index_co[index[i]]]<<std::endl;
    }
    
    std::cout.rdbuf(coutBuf);
    outFile.close();
}