#include "ssPEQT_ROT.h"
void softSend(u32 numElements, Socket &chl, PRNG& prng, AlignedVector<std::array<block, 2>> &sMsgs, u32 numThreads)
{
    SoftSpokenShOtSender<> sender;
    sender.init(fieldBits, true);
    const size_t numBaseOTs = sender.baseOtCount();
    PRNG prngOT(prng.get<block>());
    AlignedVector<block> baseMsg;
    // choice bits for baseOT
    BitVector baseChoice;

    // OTE's sender is base OT's receiver
    DefaultBaseOT base;
    baseMsg.resize(numBaseOTs);
    // randomize the base OT's choice bits
    baseChoice.resize(numBaseOTs);
    baseChoice.randomize(prngOT);
    // perform the base ot, call sync_wait to block until they have completed.
    coproto::sync_wait(base.receive(baseChoice, baseMsg, prngOT, chl));

    sender.setBaseOts(baseMsg, baseChoice);
    // perform random ots

    sMsgs.resize(numElements);
    coproto::sync_wait(sender.send(sMsgs, prngOT, chl));

}

void softRecv(u32 numElements, BitVector bitV, Socket &chl, PRNG& prng, AlignedVector<block> &rMsgs, u32 numThreads)
{

    SoftSpokenShOtReceiver<> receiver;
    receiver.init(fieldBits, true);
    const size_t numBaseOTs = receiver.baseOtCount();
    AlignedVector<std::array<block, 2>> baseMsg(numBaseOTs);
    PRNG prngOT(prng.get<block>());

    // OTE's receiver is base OT's sender
    DefaultBaseOT base;
    // perform the base ot, call sync_wait to block until they have completed.
    coproto::sync_wait(base.send(baseMsg, prngOT, chl));

    receiver.setBaseOts(baseMsg);

    rMsgs.resize(numElements);
    coproto::sync_wait(receiver.receive(bitV, rMsgs, prngOT, chl));

}

void SS_sender(Socket &chl, std::vector<block> &matrix, u32 rowNum, u32 colNum,std::vector<block> &out, u32 numThreads){
    u32 len = matrix.size();
    assert(len == rowNum * colNum);
    out.resize(rowNum);
    PRNG prng(sysRandomSeed()); 

    
    u64 keyBitLength = 40 + oc::log2ceil(len);  
    u64 keyByteLength = oc::divCeil(keyBitLength, 8); 

    oc::Matrix<u8> mLabel(len, keyByteLength);
    for(u32 i = 0; i < len; ++i){
        memcpy(&mLabel(i,0), &matrix[i], keyByteLength);
    }    
    BetaCircuit cir = isZeroCircuit(keyBitLength);
    volePSI::Gmw cmp;
    cmp.init(mLabel.rows(), cir, numThreads, 0, prng.get());
    cmp.implSetInput(0, mLabel, mLabel.cols());
    coproto::sync_wait(cmp.run(chl));

    oc::Matrix<u8> mOut;
    mOut.resize(len, 1);
    cmp.getOutput(0, mOut);
    // bitV[i] = mOut[i*colNum] ^ mOut[i*colNum + 1] ... ^ mOut[(i+1) * colNum - 1]  
    BitVector bitV(rowNum);

    for(auto i = 0; i < rowNum; ++i){
        for(auto j = 0; j < colNum; ++j){
            bitV[i] ^= (mOut(j * rowNum + i, 0) & 1); 
        }
    }

    AlignedVector<std::array<block, 2>> sMsgs(rowNum);
    softSend(rowNum, chl, prng, sMsgs, numThreads);

    for(u32 i = 0; i < rowNum; ++i){
        out[i] = sMsgs[i][bitV[i]];
    }  
}

void SS_receiver(Socket &chl, std::vector<block> &matrix, u32 rowNum, u32 colNum,std::vector<block> &out, u32 numThreads){

    u32 len = matrix.size();
    assert(len == rowNum * colNum);
    out.resize(rowNum);
    PRNG prng(sysRandomSeed()); 

    
    u64 keyBitLength = 40 + oc::log2ceil(len);  
    u64 keyByteLength = oc::divCeil(keyBitLength, 8);  

    oc::Matrix<u8> mLabel(len, keyByteLength);
    for(u32 i = 0; i < len; ++i){
        memcpy(&mLabel(i,0), &matrix[i], keyByteLength);
    }    
    BetaCircuit cir = isZeroCircuit(keyBitLength);
    volePSI::Gmw cmp;
    cmp.init(mLabel.rows(), cir, numThreads, 1, prng.get());
    cmp.setInput(0, mLabel);
    coproto::sync_wait(cmp.run(chl));

    oc::Matrix<u8> mOut;
    mOut.resize(len, 1);
    cmp.getOutput(0, mOut);
    // bitV[i] = mOut[i*colNum] ^ mOut[i*colNum + 1] ... ^ mOut[(i+1) * colNum - 1]  
    BitVector bitV(rowNum);
    
    for(auto i = 0; i < rowNum; ++i){
        for(auto j = 0; j < colNum; ++j){
            bitV[i] ^= (mOut(j * rowNum + i, 0) & 1);
            bitV[i] ^= 1;
        }
    }

    AlignedVector<block> rMsgs(rowNum);
    softRecv(rowNum, bitV, chl, prng, rMsgs, numThreads);
    memcpy(out.data(), rMsgs.data(), rowNum * sizeof(block));
}

void ssPEQT_ROT_sender(u32 numThreads,u32 tn){
    u64 item_cnt;
    u64 alpha_max_cache_count;
    std::ifstream randomMFile;

    std::vector<Socket> chl;
    for(int i=1;i<tn+1;i++){
        chl.emplace_back(coproto::asioConnect("localhost:" + std::to_string(1212 + 150+i), 1));
    }


    for (int i = 0; i < tn; i++)
    {
        std::string filePath = "../../MCRG/mcrg_result/sender_mcrg_"+std::to_string(i);
        if (!filePath.empty()){
            randomMFile.open(filePath, std::ios::binary | std::ios::in);
            if (!randomMFile.is_open()){
                //std::cout<<"failed to open"<< filePath<<std::endl;
                return;
            }
        }
        randomMFile.read((char*)(&item_cnt), sizeof(uint64_t));
        randomMFile.read((char*)(&alpha_max_cache_count), sizeof(uint64_t));
        std::vector<block> decrypt_randoms_matrix(item_cnt * alpha_max_cache_count);
        randomMFile.read((char*)decrypt_randoms_matrix.data(), sizeof(block) * decrypt_randoms_matrix.size());
        randomMFile.close();

        for(int i = 0; i < decrypt_randoms_matrix.size(); i++){
            decrypt_randoms_matrix[i] = block(0, decrypt_randoms_matrix[i].mData[0]);
        }
        std::vector<uint32_t> pi;  
        std::vector<block> MCRG_out; 


        SS_sender(chl[i], decrypt_randoms_matrix, item_cnt, alpha_max_cache_count, MCRG_out, numThreads);

        //write   
        std::string outFileName = "../mcrg_result/sender_mcrg_"+std::to_string(i);
        std::ofstream outFile;
        outFile.open(outFileName, std::ios::binary | std::ios::out);
        if (!outFile.is_open()){
            std::cout << "Vole error opening file " << outFileName << std::endl;
            return;
        }
        int numBins = MCRG_out.size();
        outFile.write((char*)&numBins, sizeof(uint64_t));
        outFile.write((char*)MCRG_out.data(), sizeof(block)*(MCRG_out.size()));
        outFile.close();
    }

    double comm = 0;

    for (u32 i = 0; i < tn; ++i)
    {
        comm += chl[i].bytesSent()+chl[i].bytesReceived();
    }
    std::cout << "Total Comm cost = " << std::fixed << std::setprecision(3) << comm / 1024 / 1024 << " MB" << std::endl;
}

void ssPEQT_ROT_receiver(int idx,u32 numThreads){

    Socket chl = coproto::asioConnect("localhost:" + std::to_string(1212 + 150+idx), 0);

    u64 item_cnt;
    u64 alpha_max_cache_count;
    std::ifstream randomMFile;
    std::string filePath = "../../MCRG/mcrg_result/receiver_0_"+std::to_string(idx-1);
    if (!filePath.empty()){
        randomMFile.open(filePath, std::ios::binary | std::ios::in);
        if (!randomMFile.is_open()){
            return;
        }
    }
    randomMFile.read((char*)(&item_cnt), sizeof(uint64_t));
    randomMFile.read((char*)(&alpha_max_cache_count), sizeof(uint64_t));

    std::vector<block> random_matrix(item_cnt * alpha_max_cache_count);
    randomMFile.read((char*)random_matrix.data(), sizeof(block) * random_matrix.size());
    randomMFile.close();

    for(int i = 0; i < random_matrix.size(); i++){
        random_matrix[i] = block(0, random_matrix[i].mData[0]);
    }       


    std::vector<block> MCRG_out; 
    SS_receiver(chl, random_matrix, item_cnt, alpha_max_cache_count, MCRG_out, numThreads);

    //write   
    std::string outFileName = "../mcrg_result/receiver_mcrg_"+std::to_string(idx-1);
    std::ofstream outFile;
    outFile.open(outFileName, std::ios::binary | std::ios::out);
    if (!outFile.is_open()){
        std::cout << "Vole error opening file " << outFileName << std::endl;
        return;
    }

    int numBins = MCRG_out.size();
    outFile.write((char*)&numBins, sizeof(uint64_t));
    outFile.write((char*)MCRG_out.data(), sizeof(block)*(MCRG_out.size()));
    outFile.close();

}
