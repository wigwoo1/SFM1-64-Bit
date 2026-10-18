#include "buffer_storage_growth_candidate.h"
#include <cassert>
#include <iostream>
struct Adapter{
 unsigned grow=0,cap=0,calls=0;unsigned char storageByte=0;unsigned char* storage=nullptr;bool fail=false;
 unsigned growSize(){return grow;}unsigned capacity(){return cap;}unsigned char* sourceData(){return storage;}
 void setCapacity(unsigned n){cap=n;}void setStorage(unsigned char* p){storage=p;}
 unsigned char* allocate(unsigned n){assert(n==cap);++calls;return fail?nullptr:&storageByte;}
 unsigned char* reallocate(unsigned char* p,unsigned n){assert(p==&storageByte);return allocate(n);}
};
int main(){unsigned cases=0;
 for(unsigned grow:{0u,1u,4u,16u,260u})for(unsigned initial=0;initial<300;++initial)for(unsigned delta:{1u,2u,31u,260u}){
 Adapter a;a.grow=grow;a.cap=initial;if(initial)a.storage=&a.storageByte;
 growBufferStorageCandidate(delta,a);unsigned expected=initial;
 if(grow)expected=((initial+delta-1)/grow+1)*grow;else{if(!expected)expected=32;while(expected<initial+delta)expected*=2;}
 assert(a.cap==expected && a.calls==1 && a.storage==&a.storageByte);++cases;
 }
 Adapter b;b.grow=0xffffffff;b.cap=7;growBufferStorageCandidate(1,b);assert(b.cap==7 && !b.calls);
 Adapter c;c.cap=32;c.storage=&c.storageByte;c.fail=true;growBufferStorageCandidate(1,c);assert(c.cap==64 && !c.storage);
 std::cout<<"x64 storage growth: "<<cases<<" bounded capacity cases, external no-op and allocator failure publication passed\n";
}
