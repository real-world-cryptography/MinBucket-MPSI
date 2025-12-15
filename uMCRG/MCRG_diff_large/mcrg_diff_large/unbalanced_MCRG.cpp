#include "ssPEQT_ROT.h"


using namespace oc;


void unbalanced_MCRG(u32 idx, u32 numThreads,u32 tn)
{
    Timer timer;
    timer.setTimePoint("start"); 
    if(idx == 0){
        ssPEQT_ROT_sender(numThreads,tn-1);

        timer.setTimePoint("MCRG"); 
        std::cout << timer << std::endl;
    }
    else{
        ssPEQT_ROT_receiver(idx,numThreads);
    }
}


int main(int agrc, char** argv){
    
    CLP cmd;
    cmd.parse(agrc, argv);
    
    // Number of threads
    u32 nt = cmd.getOr("nt", 1);

    // Participant Index (from 0 to tn-1)
    u32 idx = cmd.getOr("r", 0);

    // Total number of participants
    u32 tn = cmd.getOr("tn", 2);

    unbalanced_MCRG(idx, nt, tn);

    return 0;
}
