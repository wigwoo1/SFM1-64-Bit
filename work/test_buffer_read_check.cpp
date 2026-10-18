#include "buffer_read_check_candidate.h"
#include <cassert>
#include <iostream>
struct Adapter{unsigned char error=0;std::uint32_t get=4,max=20,offset=0,cap=10;int calls=0;bool result=false;
    auto errorByte(){return error;}void setErrorByte(unsigned char v){error=v;}
    auto getPosition(){return get;}auto maxGet(){return max;}auto bufferOffset(){return offset;}auto capacity(){return cap;}
    bool readOverflow(std::uint32_t){++calls;error|=0x40;return result;}
};
int main(){Adapter a;assert(checkBufferReadCandidate(6,a) && !a.calls && a.get==4);
    Adapter b;assert(!checkBufferReadCandidate(17,b) && b.error==2 && !b.calls);
    Adapter c;c.result=true;assert(checkBufferReadCandidate(7,c) && c.calls==1 && c.error==0x40);
    Adapter d;assert(!checkBufferReadCandidate(7,d) && d.error==0x42);assert(!checkBufferReadCandidate(1,d) && d.calls==1);
    Adapter e;e.get=0x7fffffff;e.max=0x7fffffff;e.cap=0x7fffffff;assert(checkBufferReadCandidate(1,e));
    Adapter f;assert(checkBufferReadCandidate(0xffffffff,f));
    std::cout<<"x64 buffer read check: bounds/resident/callback/live errors/no cursor advance/raw wrap/negative request passed\n";
}
