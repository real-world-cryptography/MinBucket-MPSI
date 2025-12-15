#include "JPEQT.h"

using namespace coproto;

void genPermutation(u32 size, std::vector<u32> &pi)
{
    pi.resize(size);
    for (size_t i = 0; i < pi.size(); ++i){
        pi[i] = i;
    }
    std::shuffle(pi.begin(), pi.end(), global_built_in_prg2);
}

void permute(std::vector<u32> &pi, std::vector<ExponentialElGamal::CT> &ct){
    std::vector<ExponentialElGamal::CT> res(ct.size());
    for (size_t i = 0; i < pi.size(); ++i){
        res[i] = ct[pi[i]];
    }
    ct.assign(res.begin(), res.end());
}


//  The Joint-PEQT protocol in the paper
void J_PEQT(u32 numThreads,u32 idx,ExponentialElGamal::PP& pp,u32 tn,u32 bn)
{ 

    // connection 
    std::vector<Socket> chl_0;
    std::vector<Socket> chl_1;
    for (u32 i = 0; i < tn; ++i){
        if (i < idx){
            chl_0.emplace_back(asioConnect("localhost:" + std::to_string(PORT + idx * 100 + i), true));
        } else if (i > idx){
            chl_0.emplace_back(asioConnect("localhost:" + std::to_string(PORT + i * 100 + idx), false));
        }
    }
    for (u32 i = 0; i < tn; ++i){
        if (i < idx){
            chl_1.emplace_back(asioConnect("localhost:" + std::to_string(PORT + idx * 155 + i), false));
        } else if (i > idx){
            chl_1.emplace_back(asioConnect("localhost:" + std::to_string(PORT + i * 155 + idx), true));
        }
    }

    ECPoint pk;                     
    BigInt sk;
    ECPoint pk_HE;

    //  P0 acts as a central participant
    if(idx==0)
    {
        // read mcrg result
        read_mcrg_e(tn,bn);
        Timer timer;
        timer.setTimePoint("start");

        // KeyGen
        std::tie(pk, sk) = ExponentialElGamal::KeyGen(pp);
        std::vector<ECPoint> pk_other(tn-1); 
        for(int i = 0;i<tn-1;i++){
            recvECPoint(chl_0[i],pk_other[i]);
        }
        pk_HE = pk.Add(Get_pk_HE(pk_other,tn-1));
        for(int i = 0; i < tn-1; i++){
            sendECPoint(chl_1[i], pk_HE);
        }
        
        //  P0 gets his e
        std::vector<BigInt> E(numBins);
        #pragma omp parallel for num_threads(numThreads)
        for (int j = 0; j < numBins; j++)
        {
            for (int i = 0; i < tn-1; i++)
            {
                E[j]=E[j].Add(e[i][j]);
            }
        }
        
        std::vector<ExponentialElGamal::CT> E_HE(numBins);
        BigInt rb = GenRandomBigIntLessThan(order);
        ECPoint rE = pp.g *rb;
        ECPoint zero = rE.Sub(rE);
        for (int i = 0; i < numBins; i++)
        {
            E_HE[i].X = zero; 
            E_HE[i].Y = pp.g * E[i];
        } 

        // wait D
        std::vector<std::vector<ExponentialElGamal::CT>> CT_d ;
        CT_d.resize(tn-1, std::vector<ExponentialElGamal::CT>(numBins));
        for (int i = 0; i < tn-1; i++)
        {
            recvCTs(chl_0[i], CT_d[i]);
        }

        // ReEnc Ciphertext
        std::vector<ExponentialElGamal::CT> D_HE(numBins);
        #pragma omp parallel for num_threads(numThreads)
        for(int i = 0; i < numBins; i++)
        {
            D_HE[i].X = D_HE[i].Y = zero; 
        }
        #pragma omp parallel for num_threads(numThreads)
        for (int j = 0; j < numBins; j++)
        {
            for (int i = 0; i < tn-1; i++)
            {
                D_HE[j] = ExponentialElGamal::HomoAdd(D_HE[j], CT_d[i][j]);
            }
        }
        std::vector<ExponentialElGamal::CT> ct(numBins);
        std::vector<BigInt> r_new(numBins);
        for(int i = 0; i < numBins; i++)
        {
            r_new[i] = GenRandomBigIntLessThan(order); 
        }

        #pragma omp parallel for num_threads(numThreads)
        for (int i = 0; i < numBins; i++)
        {
            ct[i] = ExponentialElGamal::HomoSub(D_HE[i], E_HE[i]);
            ct[i] = ReEnc(pp, pk_HE, ct[i], r_new[i]) ;
        }

        //  send CT to P1
        sendCTs(chl_1[0],ct);
        //  recv CT from P1
        recvCTs(chl_0[0], ct);

        //  get intersection
        std::vector<int> m_idx;
        #pragma omp parallel for num_threads(numThreads)
        for (int i = 0; i < numBins; i++)
        {
            if(Get_intersection(sk, ct[i])){
                m_idx.emplace_back(i);
            }       
        }

        timer.setTimePoint("end"); 
        std::cout << timer << std::endl;
        write_intersection(m_idx);

        double comm = 0;
        for (u32 i = 0; i < tn-1; ++i){
            
            comm += chl_0[i].bytesSent()+chl_0[i].bytesReceived();
            
        }
        for (u32 i = 0; i < tn-1; ++i){
            
            comm += chl_1[i].bytesSent()+chl_1[i].bytesReceived();
            
        }

        std::cout << "Total Comm cost = " << std::fixed << std::setprecision(3) << comm / 1024 / 1024 << " MB" << std::endl;
    }
        
    //  other parties
    else
    {
        std::tie(pk, sk) = ExponentialElGamal::KeyGen(pp);
        sendECPoint(chl_0[0], pk);

        // di and sends to 0
        recvECPoint(chl_1[0], pk_HE);
        std::vector<ExponentialElGamal::CT> ct_d;

        // BIG-SET mcrg result
        if(idx < bn+1)
        {
            read_amcrg_d(idx-1);
            ct_d.resize(numBins);
            #pragma omp parallel for num_threads(numThreads)
            for (int i = 0; i < numBins; i++)
            {
                ct_d[i]= ExponentialElGamal::Enc(pp, pk_HE, apsu_d[i]);
            } 
        }

        // Small-Set mcrg result
        if(idx > bn)
        {
            read_emcrg_d(idx - bn -1);
            ct_d.resize(numBins);
            #pragma omp parallel for num_threads(numThreads)
            for (int i = 0; i < numBins; i++)
            {
                ct_d[i]= ExponentialElGamal::Enc(pp, pk_HE, epsu_d[i]);
            } 
        }

        sendCTs(chl_0[0],ct_d);
        recvCTs(chl_1[idx-1], ct_d);

        //  ReRandom
        std::vector<BigInt> alpha(numBins);
        for (int i = 0; i < numBins; i++)
        {
            alpha[i]= GenRandomBigIntLessThan(order);
        }
        #pragma omp parallel for num_threads(numThreads)
        for (int i = 0; i < numBins; i++)
        {
            ct_d[i]= ExponentialElGamal::ScalarMul(ct_d[i], alpha[i]);
        } 

        if (idx == tn-1)
        {
            // Dec
            for (int i = 0; i < numBins; i++)
            {
                ct_d[i]= Dec(sk,ct_d[i]);
            } 
            sendCTs(chl_0[idx-1],ct_d);

        }
        else{
            sendCTs(chl_1[idx],ct_d);
            recvCTs(chl_0[idx], ct_d);
            for (int i = 0; i < numBins; i++)
            {
                ct_d[i]= Dec(sk,ct_d[i]);
            } 
            sendCTs(chl_0[idx-1],ct_d);
        }
    }

}


//  The JP-PEQT protocol in the paper
//  Added permute on the basis of JPEQT
void JP_PEQT(u32 numThreads,u32 idx,ExponentialElGamal::PP& pp,u32 tn,u32 bn)
{ 

    // connection
    std::vector<Socket> chl_0;
    std::vector<Socket> chl_1;
    for (u32 i = 0; i < tn; ++i){
        if (i < idx){
            chl_0.emplace_back(asioConnect("localhost:" + std::to_string(PORT + idx * 100 + i), true));
        } else if (i > idx){
            chl_0.emplace_back(asioConnect("localhost:" + std::to_string(PORT + i * 100 + idx), false));
        }
    }
    for (u32 i = 0; i < tn; ++i){
        if (i < idx){
            chl_1.emplace_back(asioConnect("localhost:" + std::to_string(PORT + idx * 155 + i), false));
        } else if (i > idx){
            chl_1.emplace_back(asioConnect("localhost:" + std::to_string(PORT + i * 155 + idx), true));
        }
    }

    
    ECPoint pk;                
    BigInt sk;
    ECPoint pk_HE;
    std::vector<u32> pi;// for permute

    if(idx==0)
    {
        read_mcrg_e(tn,bn);
        Timer timer;
        timer.setTimePoint("start");
        std::tie(pk, sk) = ExponentialElGamal::KeyGen(pp);
        std::vector<ECPoint> pk_other(tn-1); 
        for(int i = 0;i<tn-1;i++){
            recvECPoint(chl_0[i],pk_other[i]);
        }


        pk_HE = pk.Add(Get_pk_HE(pk_other,tn-1));

        for(int i = 0; i < tn-1; i++){
            sendECPoint(chl_1[i], pk_HE);
        }
        std::vector<BigInt> E(numBins);
        #pragma omp parallel for num_threads(numThreads)
        for (int j = 0; j < numBins; j++)
        {
            for (int i = 0; i < tn-1; i++)
            {
                E[j]=E[j].Add(e[i][j]);
            }
        }
        
        std::vector<ExponentialElGamal::CT> E_HE(numBins);
        BigInt rb = GenRandomBigIntLessThan(order);
        ECPoint rE = pp.g *rb;
        ECPoint zero = rE.Sub(rE);
        for (int i = 0; i < numBins; i++)
        {
            E_HE[i].X = zero; 
            E_HE[i].Y = pp.g * E[i];
        } 

        std::vector<std::vector<ExponentialElGamal::CT>> CT_d ;
        CT_d.resize(tn-1, std::vector<ExponentialElGamal::CT>(numBins));
        for (int i = 0; i < tn-1; i++)
        {
            recvCTs(chl_0[i], CT_d[i]);
        }

        std::vector<ExponentialElGamal::CT> D_HE(numBins);
        #pragma omp parallel for num_threads(numThreads)
        for(int i = 0; i < numBins; i++)
        {
            D_HE[i].X = D_HE[i].Y = zero; 
        }
        
        #pragma omp parallel for num_threads(numThreads)
        for (int j = 0; j < numBins; j++)
        {
            for (int i = 0; i < tn-1; i++)
            {
                D_HE[j] = ExponentialElGamal::HomoAdd(D_HE[j], CT_d[i][j]);
            }
        }


        std::vector<ExponentialElGamal::CT> ct(numBins);
        std::vector<BigInt> r_new(numBins);
        for(int i = 0; i < numBins; i++)
        {
            r_new[i] = GenRandomBigIntLessThan(order); 
        }
        #pragma omp parallel for num_threads(numThreads)
        for (int i = 0; i < numBins; i++)
        {
            ct[i] = ExponentialElGamal::HomoSub(D_HE[i], E_HE[i]);
            ct[i] = ReEnc(pp, pk_HE, ct[i], r_new[i]) ;
        }

        sendCTs(chl_1[0],ct);
        recvCTs(chl_0[0], ct);

        std::vector<int> m_idx;
        #pragma omp parallel for num_threads(numThreads)
        for (int i = 0; i < numBins; i++)
        {
            if(Get_intersection(sk, ct[i])){
                m_idx.emplace_back(i);
            }       
        }

        timer.setTimePoint("end"); 
        std::cout << timer << std::endl;

        std::cout<<"The size of the intersection of "<<tn<<" parties is:"<<m_idx.size()<<std::endl;

        double comm = 0;
        for (u32 i = 0; i < tn-1; ++i){
            
            comm += chl_0[i].bytesSent()+chl_0[i].bytesReceived();
            
        }
        for (u32 i = 0; i < tn-1; ++i){
            
            comm += chl_1[i].bytesSent()+chl_1[i].bytesReceived();
            
        }

        std::cout << "Total Comm cost = " << std::fixed << std::setprecision(3) << comm / 1024 / 1024 << " MB" << std::endl;
    }
        
    else
    {
        std::tie(pk, sk) = ExponentialElGamal::KeyGen(pp);
        sendECPoint(chl_0[0], pk);

        recvECPoint(chl_1[0], pk_HE);
        std::vector<ExponentialElGamal::CT> ct_d;

        if(idx < bn+1)
        {
            read_amcrg_d(idx-1);
            ct_d.resize(numBins);
            #pragma omp parallel for num_threads(numThreads)
            for (int i = 0; i < numBins; i++)
            {
                ct_d[i]= ExponentialElGamal::Enc(pp, pk_HE, apsu_d[i]);
            } 
        }


        if(idx > bn)
        {
            read_emcrg_d(idx - bn -1);
            ct_d.resize(numBins);
            #pragma omp parallel for num_threads(numThreads)
            for (int i = 0; i < numBins; i++)
            {
                ct_d[i]= ExponentialElGamal::Enc(pp, pk_HE, epsu_d[i]);
            } 
        }

        sendCTs(chl_0[0],ct_d);

        recvCTs(chl_1[idx-1], ct_d);

        //  Rerandom
        std::vector<BigInt> alpha(numBins);
        for (int i = 0; i < numBins; i++)
        {
            alpha[i]= GenRandomBigIntLessThan(order);
        }
        #pragma omp parallel for num_threads(numThreads)
        for (int i = 0; i < numBins; i++)
        {
            ct_d[i]= ExponentialElGamal::ScalarMul(ct_d[i], alpha[i]);
        } 

        // get permute
        genPermutation(numBins, pi);

        if (idx == tn-1)
        {
            for (int i = 0; i < numBins; i++)
            {
                ct_d[i]= Dec(sk,ct_d[i]);
            } 
            permute(pi, ct_d);// perimutation
            sendCTs(chl_0[idx-1],ct_d);

        }
        else{
            sendCTs(chl_1[idx],ct_d);
            recvCTs(chl_0[idx], ct_d);
            for (int i = 0; i < numBins; i++)
            {
                ct_d[i]= Dec(sk,ct_d[i]);
            } 
            permute(pi, ct_d);
            sendCTs(chl_0[idx-1],ct_d);
        }
    }

}

int main(int agrc, char** argv){


    CLP cmd;
    cmd.parse(agrc, argv);
    // Number of threads
    u32 nt = cmd.getOr("nt", 1);

    // Participant Index(from 0 to tn-1)
    u32 idx = cmd.getOr("r", 0);

    // Total number of participants
    u32 tn = cmd.getOr("tn", 2);

    // The number of large-set participants 
    u32 bn = cmd.getOr("bn", 1);

    // J-PEQT is 0 and JP-PEQT is 1
    int permu = cmd.getOr("p", 0);


    CRYPTO_Initialize();  
    std::ios::sync_with_stdio(false);
    u32 numThreads = nt;
    ExponentialElGamal::PP pp = ExponentialElGamal::Setup(MSG_LEN, TRADEOFF_NUM);

    if(permu == 1){
        JP_PEQT(nt,idx,pp,tn,bn);
    }
    else{
        J_PEQT(nt,idx,pp,tn,bn); 
    }

    CRYPTO_Finalize(); 
}
