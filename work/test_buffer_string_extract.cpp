#include "buffer_string_extract_candidate.h"
#include <cstring>
#include <cassert>
#include <iostream>
#include <vector>
struct Adapter{
    std::vector<std::uint8_t> bytes;std::vector<std::uint32_t> requests;
    std::uint8_t error=0,flags=1;std::uint32_t get=0,offset=0,max=0,cap=0;
    bool refill=true;std::uint8_t refillError=0;
    explicit Adapter(std::vector<std::uint8_t> b):bytes(b),max(static_cast<std::uint32_t>(b.size())),cap(max){}
    auto errorByte(){return error;}void setErrorByte(std::uint8_t e){error=e;}
    void setGetPosition(std::uint32_t v){get=v;}
    void copyOutput(std::uint8_t* dst,const std::uint8_t* src,std::uint32_t n){std::memmove(dst,src,n);}
    auto flagsByte(){return flags;}auto getPosition(){return get;}auto maxGet(){return max;}
    auto bufferOffset(){return offset;}auto capacity(){return cap;}
    bool readOverflow(std::uint32_t n){requests.push_back(n);cap=max;error|=refillError;return refill;}
    auto sourceByte(std::uint32_t i){assert(i<bytes.size());return bytes[i];}
    auto sourceData(){return bytes.data();}
};
int main(){
    std::uint8_t out[32];
    Adapter a({32,9,'a','b','c',32,'d',0});
    extractBufferStringCandidate(out,32,a);assert(std::strcmp(reinterpret_cast<char*>(out),"abc")==0 && a.get==5 && !a.error);
    extractBufferStringCandidate(out,32,a);assert(out[0]=='d' && out[1]==0 && a.get==7);
    for(unsigned cap: {0u,1u,2u,3u,4u,5u}){
        Adapter b({'a','b','c',0});extractBufferStringCandidate(out,cap,b);
        auto copied=cap==0?3u:(cap<4?cap-1:3u);assert(out[copied]==0 && b.get==3 && !b.error);
        for(unsigned i=0;i<copied;++i)assert(out[i]==static_cast<std::uint8_t>('a'+i));
        Adapter c({'a','b','c',0,'x',0});c.flags=0;extractBufferStringCandidate(out,cap,c);assert(c.get==4 && !c.error);
        extractBufferStringCandidate(out,32,c);assert(out[0]=='x' && out[1]==0 && c.get==6);
    }
    Adapter d({32,9});extractBufferStringCandidate(out,32,d);assert(!out[0] && d.get==2 && d.error==2);
    Adapter e({'a'});e.error=1;extractBufferStringCandidate(out,32,e);assert(!out[0] && !e.get && e.error==1);
    Adapter f({'a','b'});f.flags=0;extractBufferStringCandidate(out,32,f);assert(out[0]=='a' && out[1]=='b' && out[2]==0 && f.get==2 && f.error==2);
    Adapter overlap({'a','b','c',0,0,0,0,0});extractBufferStringCandidate(overlap.bytes.data()+1,4,overlap);assert(overlap.get==3 && overlap.bytes[1]=='a' && overlap.bytes[2]=='b' && overlap.bytes[3]=='c' && overlap.bytes[4]==0);
    std::cout<<"x64 composed extraction: repeated text/binary tokens, zero/one/exact/truncated capacities, whitespace-only and unterminated input passed\n";
}
