#include "buffer_default_overflow_candidate.h"
#include <cassert>
#include <iostream>
struct Adapter{
 unsigned grow=0,flags=0,cap=1,put=0,offset=0,calls=0,converted=0;bool mutate=false;
 unsigned growSize(){return grow;}unsigned flagsByte(){return flags;}unsigned capacity(){return cap;}unsigned putPosition(){return put;}unsigned bufferOffset(){return offset;}
 void convertExternalStorage(unsigned n){assert(n==0);++converted;grow=0;}
 void growStorage(unsigned n){assert(n==1);++calls;cap*=2;if(mutate){put=offset=7;}assert(calls<32);}
};
int main(){
 for(unsigned n:{0u,1u,128u,0xffffffffu})assert(!defaultBufferReadOverflowCandidate(n));
 Adapter a;assert(defaultBufferWriteOverflowCandidate(260,a));assert(a.cap==512 && a.calls==9);
 Adapter b;b.grow=0xffffffff;assert(!defaultBufferWriteOverflowCandidate(1,b) && !b.calls && !b.converted);
 Adapter c;c.grow=0xffffffff;c.flags=2;assert(defaultBufferWriteOverflowCandidate(3,c) && c.converted==1 && c.calls==2);
 Adapter d;d.put=100;d.mutate=true;assert(defaultBufferWriteOverflowCandidate(1,d) && d.calls==1 && d.cap==2);
 Adapter e;assert(defaultBufferWriteOverflowCandidate(0xffffffff,e) && !e.calls);
 std::cout<<"x64 default overflow: read refusal, repeated growth, external gate/conversion, live mutation and signed request passed\n";
}
