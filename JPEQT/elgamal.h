#pragma once
#include "../Kunlun/crypto/setup.hpp"
#include"../Kunlun/pke/exponential_elgamal.hpp"
#include "../Kunlun/pke/calculate_dlog.hpp"



// Ciphertext re-randomization
ExponentialElGamal::CT ReEnc(const ExponentialElGamal::PP &pp, const ECPoint &pk, const ExponentialElGamal::CT &c, const BigInt &r)
{ 
    ExponentialElGamal::CT ct; 
    // begin encryption
    ct.X = pp.g * r; // X = g^r
    ct.X = ct.X+c.X; //X = g^(r+r1+r2) 
    ct.Y = pk * r + c.Y; // CT_new.Y = pk^r + pk^(r1+r2)+g^m
    return ct; 
}

// Incomplete decryption
ExponentialElGamal::CT Dec(const BigInt& sk, const ExponentialElGamal::CT &ct)
{ 
    ExponentialElGamal::CT ct_new; 
    ct_new.Y = ct.Y - ct.X * sk;
    ct_new.X = ct.X;
    return ct_new; 
}


// Threshold encryption key
ECPoint Get_pk_HE(std::vector<ECPoint> pk_other,int num){
    ExponentialElGamal::CT ct;
    ct.Y = pk_other[0];
    for (int i = 1; i < num; i++)
    {
       ct.Y = ct.Y + pk_other[i];
    }
    return ct.Y;
}

// The elements that meet the conditions are the intersection elements
bool Get_intersection(const BigInt& sk, const ExponentialElGamal::CT &ct)
{ 
    if(ct.X*sk == ct.Y){
        return true;
    }
    else{
        return false;
    }
}
