#include "element_fixed16_copy_candidate.h"
#include <array>
#include <cassert>
#include <iostream>
int main(){
    std::array<unsigned char,40> data{};
    auto reset=[&](){for(unsigned i=0;i<data.size();++i)data[i]=static_cast<unsigned char>(i);};
    reset();copyElementFixed16Candidate(data.data()+1,data.data()+21);
    for(unsigned i=0;i<16;++i)assert(data[21+i]==1+i);
    assert(data[20]==20 && data[37]==37);
    reset();copyElementFixed16Candidate(data.data(),data.data()+8);
    for(unsigned i=0;i<8;++i)assert(data[8+i]==i && data[16+i]==i);
    reset();copyElementFixed16Candidate(data.data()+8,data.data());
    for(unsigned i=0;i<16;++i)assert(data[i]==8+i);
    reset();copyElementFixed16Candidate(data.data()+1,data.data()+1);
    for(unsigned i=0;i<data.size();++i)assert(data[i]==i);
    std::cout<<"x64 fixed16 copy: unaligned/forward overlap/backward overlap/self/adjacent bytes passed\n";
}
