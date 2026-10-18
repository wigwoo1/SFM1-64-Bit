#include "buffer_external_conversion_candidate.h"
#include <cassert>
#include <iostream>
struct Adapter{
 unsigned grow=0xffffffff,cap=4,calls=0;unsigned char borrowed[4]={1,2,3,4},changed[4]={5,6,7,8},owned[4]={};unsigned char* storage=borrowed;bool mutate=false;
 unsigned growSize(){return grow;}unsigned capacity(){return cap;}void setGrowSize(unsigned v){grow=v;}
 unsigned char* sourceData(){return storage;}void setStorage(unsigned char* p){storage=p;}
 unsigned char* allocate(unsigned n){assert(n==4 && grow==0);++calls;if(mutate){storage=changed;cap=2;}return owned;}
};
int main(){Adapter a;convertExternalBufferCandidate(0,a);assert(a.storage==a.owned && a.grow==0 && a.calls==1 && a.owned[3]==4 && a.borrowed[3]==4);
 Adapter b;b.grow=7;convertExternalBufferCandidate(0,b);assert(b.grow==7 && b.storage==b.borrowed && !b.calls);
 Adapter c;c.cap=0;convertExternalBufferCandidate(0,c);assert(!c.storage && !c.calls && !c.grow);
 Adapter d;d.mutate=true;convertExternalBufferCandidate(0,d);assert(d.owned[0]==5 && d.owned[3]==8 && d.cap==2 && d.storage==d.owned);
 std::cout<<"x64 external conversion: borrowed copy, owned no-op, zero capacity and live source/snapshot size passed\n";
}
