#include "MCRG.h"

void MCRG_sender(u32 numElements, std::vector<Socket> &chl_send,std::vector<Socket> &chl_recv, u32 numThreads,u32 tn){


    block cuckooSeed = block(0x235677879795a931, 0x784915879d3e658a); 
    PRNG prng(sysRandomSeed());
    block hashSeed = block(0x12387ab67853d29e, 0x58735185628bfea4);

    Baxos mPaxos;
    mPaxos.init(3 * numElements, binSize, 3, ssp, PaxosParam::GF128, block(0,0));
    u32 okvs_size = mPaxos.size();
    std::vector<block> P(okvs_size);
    //read item
    std::string filePath = "../../uMCRG/MCRG/mcrg_result/sender_item";
    std::ifstream readItem;
    if (!filePath.empty()){
        readItem.open(filePath, std::ios::binary | std::ios::in);
        if (!readItem.is_open()){
            std::cerr << "Error: Failed to open file: " << filePath << std::endl;
            return;
        }
        
    }
    uint64_t len;
    int flag;
    std::vector<block> originitem;
    std::vector<int> index_co;
    
    readItem.read((char*)(&len), sizeof(uint64_t));   
    originitem.resize(len);
    readItem.read((char*)originitem.data(), sizeof(block)*(originitem.size()));
    readItem.read((char*)(&flag), sizeof(int));
    readItem.read((char*)(&numBins), sizeof(uint64_t));
    index_co.resize(numBins);
    readItem.read((char*)index_co.data(), sizeof(int)*(index_co.size()));
    readItem.close();

    std::vector<block> s_lable(numBins);

    //prepare
    std::vector<oc::SilentVoleReceiver<block, block, oc::CoeffCtxGF128>> mVoleRecver(tn);
    std::vector<AlignedUnVector<block>> mA;
    std::vector<AlignedUnVector<block>> mC;
    mA.resize(tn,AlignedUnVector<block>(numBins));
    mC.resize(tn,AlignedUnVector<block>(numBins));

    oc::AES hasher;
    hasher.setKey(cuckooSeed);
    std::vector<std::vector<block>> diffC; 
    std::vector<std::vector<block>> keys;
    std::vector<std::vector<block>> values;
    diffC.resize(tn,std::vector<block>(numBins));
    keys.resize(tn,std::vector<block>(numBins));
    values.resize(tn,std::vector<block>(numBins));

    for (int i = 0; i < tn; i++)
    {
        //std::cout<<i<<" begin"<<std::endl;
        mVoleRecver[i].mMalType = SilentSecType::SemiHonest;
        mVoleRecver[i].configure(numBins, SilentBaseType::Base);
        coproto::sync_wait(mVoleRecver[i].silentReceive(mC[i], mA[i], prng, chl_recv[i]));

        // establish cuckoo hash table, compute diffC cuckooHashTable

        for (u32 k = 0;  k< numBins; ++k) 
        {
            if (index_co[k]!=-1)
            {
                auto j = index_co[k]; 
                block xj = block(originitem[j].mData[0], k);//compute x||z             
                keys[i][k] = xj;  
                diffC[i][k] = xj ^ mC[i][k];                       
            }
            else
            {          	          	
                keys[i][k] = prng.get(); 
                diffC[i][k] = mC[i][k];
            }
        }


        coproto::sync_wait(chl_send[i].send(diffC[i]));
        coproto::sync_wait(chl_recv[i].recv(P));
        mPaxos.decode<block>(keys[i], values[i], P, 1); 

        for (u32 k = 0; k < numBins; ++k)
        {
            s_lable[k] = hasher.hashBlock(mA[i][k]) ^ values[i][k];       
        }     

        //write   
        std::string outFileName = "../mcrg/result/sender_result_"+std::to_string(i);
        std::ofstream outFile;
        outFile.open(outFileName, std::ios::binary | std::ios::out);
        if (!outFile.is_open()){
            std::cout << "Vole error opening file " << outFileName << std::endl;
            return;
        }
        outFile.write((char*)&numBins, sizeof(uint64_t));
        outFile.write((char*)s_lable.data(), sizeof(block)*(s_lable.size()));
        outFile.close();
    }

}



void MCRG_receiver(u32 idx,u32 numElements, Socket &chl_send,Socket &chl_recv, u32 numThreads)
{

    block cuckooSeed = block(0x235677879795a931, 0x784915879d3e658a); 

    PRNG prng(sysRandomSeed());
    block hashSeed = block(0x12387ab67853d29e, 0x58735185628bfea4);

    Baxos mPaxos;
    mPaxos.init(3*numElements, binSize, 3, ssp, PaxosParam::GF128, block(0,0));
    u32 okvs_size = mPaxos.size();
    
    std::string filePath = "../../uMCRG/MCRG/hash_result/receiver_1_"+std::to_string(idx-1);//receiver_1_i-1
    std::ifstream readItem;
    if (!filePath.empty()){
        readItem.open(filePath, std::ios::binary | std::ios::in);
        if (!readItem.is_open()){
            std::cerr << "Error: Failed to open file: " << filePath << std::endl;
            return;
        }
        
    }
    uint64_t len;
    std::vector<block> originitem;
    std::vector<std::vector<int>> index_co;
    int flag;
    readItem.read((char*)(&len), sizeof(uint64_t));   
    originitem.resize(len);
    readItem.read((char*)originitem.data(), sizeof(block)*(originitem.size()));
    readItem.read((char*)(&flag), sizeof(int));

    size_t outerSize;
    readItem >> outerSize;
    std::vector<std::vector<std::pair<size_t, size_t>>> item_hash_indices(outerSize);
    for (size_t i = 0; i < outerSize; i++) 
    {
        size_t innerSize;
        readItem >> innerSize;
        std::vector<std::pair<size_t, size_t>> inner(innerSize);
        for (size_t j = 0; j < innerSize; j++) {
            size_t first, second;
            readItem >> first >> second;
            inner[j] = std::make_pair(first, second);
        }
        item_hash_indices[i] = inner;
    }
    readItem.close();

    numBins = outerSize;
    std::vector<block> t_lable(numBins);
    std::vector<block> P(okvs_size); 
    std::vector<u32> pi(numBins);


    block mD = prng.get();
    oc::SilentVoleSender<block,block, oc::CoeffCtxGF128> mVoleSender;
    mVoleSender.mMalType = SilentSecType::SemiHonest;
    mVoleSender.configure(outerSize, SilentBaseType::Base);
    AlignedUnVector<block> mB(outerSize);
    coproto::sync_wait(mVoleSender.silentSend(mD, mB, prng, chl_send));

    // diffC received
    std::vector<block> diffC(outerSize);
    coproto::sync_wait(chl_recv.recv(diffC));  
    oc::AES hasher;
    hasher.setKey(cuckooSeed);


    std::vector<block> keys(3*numElements);
    std::vector<block> values(3*numElements);
    u32 countV = 0;
    prng.get(t_lable.data(), outerSize);
    prng.get(keys.data(), keys.size());

        
    for (u32 i = 0; i < outerSize; ++i)
    {

    	auto bin = item_hash_indices[i];
    	auto size = bin.size();
    	    
    	for (u32 p = 0; p < size; ++p)
    	{
    	    	
    	    auto b = bin[p].first;
            auto j = bin[p].second;
   	    	
            block yj = block(originitem[b].mData[0], j);//compute y||j
            keys[countV] = yj; 

            yj ^= diffC[i];
            auto tmp = mB[i] ^ (yj.gf128Mul(mD));
            tmp = hasher.hashBlock(tmp);

            values[countV] = tmp ^ t_lable[i];  
            countV += 1;   	    
    	}      	        	        	
    } 

    mPaxos.solve<block>(keys, values, P, nullptr, 1);
    coproto::sync_wait(chl_send.send(P));

    std::string outFileName = "../mcrg/result/receiver_result_"+std::to_string(idx-1);
    std::ofstream outFile;
    outFile.open(outFileName, std::ios::binary | std::ios::out);
    if (!outFile.is_open()){
        std::cout << "Vole error opening file " << outFileName << std::endl;
        return;
    }
    outFile.write((char*)(&numBins), sizeof(uint64_t));
    outFile.write((char*)t_lable.data(), sizeof(block)*(t_lable.size()));
    outFile.close();
}    
