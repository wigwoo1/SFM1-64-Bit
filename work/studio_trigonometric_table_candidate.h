#pragma once
#include <array>
template<class Ops>void studioInitializeTrigonometricTableCandidate(std::array<float,256>& table,const std::array<double,3>& constants,Ops& ops){
 for(unsigned i=0;i<256;++i){double angle=static_cast<double>(i);angle*=constants[0];angle*=constants[1];angle*=constants[2];table[i]=static_cast<float>(ops.sine(angle));}
}
