#include "graph_reset_candidate.h"
#include <cassert>
#include <iostream>
struct Adapter{unsigned state,mode=3,other=9,byte=0,rectCalls=0,stateCalls=0,finalCalls=0;std::array<int,4> rect{};
 unsigned state168(){return state;}unsigned mode174(){return mode;}
 int x19c(){return 50;}int y1a0(){return -10;}int x1ac(){return 20;}int y1b0(){return 30;}
 void setRect178(std::array<int,4> r){rect=r;}void finishRect100d3250(unsigned m){assert(state==1 && m==3);++rectCalls;mode=77;}
 void finishState100c26b0(){assert(state==4 || state==5);++stateCalls;}
 void setState168(unsigned n){state=n;}void setMode174(unsigned n){mode=n;}void setState16c(unsigned n){other=n;}void setByte170(unsigned n){byte=n;}
 void finalImport(){assert(!state && !mode && !other && byte==1);++finalCalls;}
};
int main(){for(unsigned state=0;state<8;++state){Adapter a{state};graphResetCandidate(a);assert(a.rectCalls==unsigned(state==1) && a.stateCalls==unsigned(state==4 || state==5) && a.finalCalls==1);if(state==1)assert((a.rect==std::array<int,4>{18,-12,34,44}));}assert(graphResetSigned(0x80000000u)==INT32_MIN);std::cout<<"x64 graph reset: padded rectangle, state branches, live callback mutation and final state clearing passed\n";}
