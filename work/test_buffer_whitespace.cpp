#include "buffer_whitespace_candidate.h"
#include <cassert>
#include <iostream>
struct Adapter{
    unsigned char error=0,flags=1,bytes[8]={9,10,11,12,13,32,'x',0};
    std::uint32_t get=0,max=7,offset=0,cap=8;bool refill=false;int calls=0;
    auto errorByte(){return error;}void setErrorByte(unsigned char v){error=v;}
    auto flagsByte(){return flags;}auto getPosition(){return get;}
    void setGetPosition(std::uint32_t v){get=v;}auto maxGet(){return max;}
    auto bufferOffset(){return offset;}auto capacity(){return cap;}
    bool readOverflow(std::uint32_t n){assert(n==1);++calls;if(!refill)return false;get=offset=1;cap=8;error=2;return true;}
    auto sourceByte(std::uint32_t i){assert(i<8);return bytes[i];}
};
int main(){
    Adapter a;skipBufferWhitespaceCandidate(a);assert(a.get==6 && !a.error);
    for(unsigned v=0;v<256;++v){Adapter b;b.bytes[0]=static_cast<unsigned char>(v);b.bytes[1]='x';skipBufferWhitespaceCandidate(b);assert(b.get==(((v>=9 && v<=13)||v==32)?1u:0u));}
    Adapter b;b.flags=0;skipBufferWhitespaceCandidate(b);assert(!b.get);
    Adapter c;c.error=1;skipBufferWhitespaceCandidate(c);assert(!c.get && c.error==1);
    Adapter d;d.max=6;skipBufferWhitespaceCandidate(d);assert(d.get==6 && d.error==2);
    Adapter e;e.offset=1;e.refill=true;skipBufferWhitespaceCandidate(e);assert(e.calls==1 && e.get==2 && e.error==2);
    Adapter f;f.offset=1;skipBufferWhitespaceCandidate(f);assert(f.calls==1 && !f.get && f.error==2);
    std::cout<<"x64 whitespace: all 256 bytes, gates, exhaustion and live refill passed\n";
}
