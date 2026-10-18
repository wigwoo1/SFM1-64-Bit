#include "native_buffer_composition_candidate.h"
#include <string>
#include <iostream>
// Fixture load simulates a service filling the buffer; not a recovered Put API.
void fixtureLoad(NativeBufferCompositionCandidate& b,const std::string& text){auto n=static_cast<std::uint32_t>(text.size());assert(checkBufferWriteCandidate(n,b));std::memmove(b.sourceData()+b.putPosition(),text.data(),n);b.setPutPosition(b.putPosition()+n);b.terminateBuffer();}
int main(){static_assert(sizeof(void*)==8);
 for(unsigned initial:{0u,1u,260u}){
 NativeBufferCompositionCandidate b(0,initial,1);assert(b.maxGet()==0);std::string token(4097,'x');fixtureLoad(b," \t"+token+"\nnext");assert(b.capacity()>=4105 && b.sourceByte(b.putPosition())==0);
 std::uint8_t out[5000];extractBufferStringCandidate(out,5000,b);assert(std::string(reinterpret_cast<char*>(out))==token && b.getPosition()==4099);
 extractBufferStringCandidate(out,3,b);assert(std::string(reinterpret_cast<char*>(out))=="ne" && b.getPosition()==4104 && !b.errorByte());
 extractBufferStringCandidate(out,5000,b);assert(!out[0] && b.errorByte()==2);
 }
 NativeBufferCompositionCandidate fixed(16,0,0);fixtureLoad(fixed,std::string("abc\0xyz\0",8));std::uint8_t out[32];extractBufferStringCandidate(out,32,fixed);assert(std::string(reinterpret_cast<char*>(out))=="abc" && fixed.getPosition()==4);extractBufferStringCandidate(out,32,fixed);assert(std::string(reinterpret_cast<char*>(out))=="xyz" && fixed.getPosition()==8);
 std::cout<<"native x64 buffer composition: real allocations, 4KiB growth, repeated text/binary extraction, truncation and exhaustion passed\n";
}
