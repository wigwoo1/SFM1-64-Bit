#include "buffer_byte_read_candidate.h"
#include <cassert>
#include <iostream>
struct Adapter{unsigned char error=0;std::uint32_t get=0,max=1,offset=0,cap=1;unsigned char bytes[2]={42,99};bool refill=false;
    auto errorByte(){return error;}void setErrorByte(unsigned char v){error=v;}
    auto getPosition(){return get;}void setGetPosition(std::uint32_t v){get=v;}
    auto maxGet(){return max;}auto bufferOffset(){return offset;}auto capacity(){return cap;}
    bool readOverflow(std::uint32_t){if(!refill)return false;offset=get=1;cap=1;return true;}
    unsigned char sourceByte(std::uint32_t i){assert(i<2);return bytes[i];}
};
int main(){Adapter a;unsigned char out=0;readBufferByteCandidate(&out,a);assert(out==42 && a.get==1);readBufferByteCandidate(&out,a);assert(out==0 && a.get==1 && a.error==2);
    Adapter b;b.get=1;b.offset=2;b.max=3;b.refill=true;readBufferByteCandidate(&out,b);assert(out==42 && b.get==2 && b.offset==1);
    Adapter c;c.error=2;readBufferByteCandidate(&out,c);assert(!out && !c.get);
    std::cout<<"x64 buffer byte read: success advance/failure zero/no advance/live refill fields passed\n";
}
