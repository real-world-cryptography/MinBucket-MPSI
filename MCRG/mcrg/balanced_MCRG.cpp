#include "MCRG.h"

using namespace oc;



void balanced_MCRG(u32 idx, u32 logNum, u32 numThreads,u32 tn){
    u32 numElements = 1 << logNum;
    PRNG prng(sysRandomSeed());
    std::vector<block> inputSet(numElements);
    std::vector<block> permutedX0;
    std::vector<block> pmcrg_out;
    prng.get(inputSet.data(), inputSet.size());
    Timer timer;
    timer.setTimePoint("start"); 
    
    if (idx == 0)
    {
        std::vector<Socket> chl_recv;
        std::vector<Socket> chl_send;
        for (u32 i = 1; i < tn+1; ++i){
            chl_recv.emplace_back(coproto::asioConnect("localhost:" + std::to_string(PORT + i * 100 ), false));//chl[0]=100 chl[1]=200
        } 

        for (u32 i = 1; i < tn+1; ++i){
            chl_send.emplace_back(coproto::asioConnect("localhost:" + std::to_string(PORT + i * 155 ), true));//chl[0]=150 chl[1]=300
        }

        MCRG_sender(numElements, chl_send, chl_recv,numThreads,tn);

        timer.setTimePoint("MCRG"); 
        std::cout << timer << std::endl;

        double comm = 0;

        for (u32 i = 0; i < tn; ++i)
        {
            comm += chl_send[i].bytesSent()+chl_send[i].bytesReceived();
        }
        std::cout << "Total Comm cost = " << std::fixed << std::setprecision(3) << comm / 1024 / 1024 << " MB" << std::endl;
    
    }
    else{
        Socket chl_send;
        chl_send = coproto::asioConnect("localhost:" + std::to_string(PORT + idx*100), true);//chl=100 chl=200
        Socket chl_recv;
        chl_recv = coproto::asioConnect("localhost:" + std::to_string(PORT + idx*155), false);//chl=100 chl=200
        MCRG_receiver(idx,numElements,chl_send,chl_recv, numThreads);
        coproto::sync_wait(chl_send.flush());
    }
}


int main(int agrc, char** argv){
    
    CLP cmd;
    cmd.parse(agrc, argv);

    // The size of a small set
    u32 nn = cmd.getOr("nn", 10);

    // Number of threads
    u32 nt = cmd.getOr("nt", 1);

    // Participant Index (from 0 to tn-1)
    u32 idx = cmd.getOr("r", 0);

    // Total number of participants
    u32 tn = cmd.getOr("tn",2);

    balanced_MCRG(idx, nn, nt,tn-1);

    return 0;
}
