#include "buffer_string_length_candidate.h"
#include <cassert>
#include <iostream>
#include <vector>
struct Adapter{
    std::vector<std::uint8_t> bytes;std::vector<std::uint32_t> requests;
    std::uint8_t error=0,flags=1;std::uint32_t get=0,offset=0,max=0,cap=0;
    bool refill=true;std::uint8_t refillError=0;
    explicit Adapter(std::vector<std::uint8_t> b):bytes(b),max(static_cast<std::uint32_t>(b.size())),cap(max){}
    auto errorByte(){return error;}void setErrorByte(std::uint8_t e){error=e;}
    auto flagsByte(){return flags;}auto getPosition(){return get;}auto maxGet(){return max;}
    auto bufferOffset(){return offset;}auto capacity(){return cap;}
    bool readOverflow(std::uint32_t n){requests.push_back(n);cap=max;error|=refillError;return refill;}
    auto sourceByte(std::uint32_t i){assert(i<bytes.size());return bytes[i];}
    auto sourceData(){return bytes.data();}
};
int main(){
    Adapter a({32,9,'a','b','c',32,'d',0});assert(bufferStringLengthCandidate(a)==4 && a.get==0);
    a.flags=0;assert(bufferStringLengthCandidate(a)==8 && a.get==0);
    Adapter b({32,9});assert(countBufferWhitespaceCandidate(0,b)==2 && !b.error);assert(bufferStringLengthCandidate(b)==0 && !b.error);
    Adapter c({'a','b'});assert(bufferStringLengthCandidate(c)==3 && !c.get);
    Adapter d({0});assert(bufferStringLengthCandidate(d)==1);d.error=1;assert(bufferStringLengthCandidate(d)==0);
    for(unsigned boundary: {127u,128u,129u,255u,256u,257u}){
        std::vector<std::uint8_t> data(boundary,'a');data.push_back(0);Adapter e(data);e.cap=1;
        assert(bufferStringLengthCandidate(e)==boundary+1 && e.get==0 && !e.error);
        assert(e.requests.size()==1 && e.requests[0]==128);
    }
    Adapter f({'a','b','c',0});f.cap=0;f.refillError=2;assert(bufferStringLengthCandidate(f)==4 && !f.error);
    Adapter g({32,'x'});g.cap=0;g.refill=false;assert(countBufferWhitespaceCandidate(0,g)==0 && !g.error && g.requests[0]==1);
    for(unsigned ch=0;ch<256;++ch){Adapter h({'a',static_cast<std::uint8_t>(ch),'b',0});const auto expected=(ch==0 || bufferWhitespaceByteCandidate(static_cast<std::uint8_t>(ch)))?2u:4u;assert(bufferStringLengthCandidate(h)==expected);}
    std::cout<<"x64 string lookahead: 256 delimiters, 128-byte boundaries, text/binary, cursor preservation and refill error clearing passed\n";
}
