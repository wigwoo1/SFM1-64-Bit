#pragma once
#include <cstdint>
template<class Adapter>
void constructOwnedBufferCandidate(std::uint32_t grow,std::uint32_t capacity,std::uint32_t flags,Adapter& a){
    a.setStorage(nullptr);a.setCapacity(capacity);a.setGrowSize(grow);
    if(capacity)a.setStorage(a.allocate(capacity));
    a.setErrorByte(0);a.setState2c(a.state2c()&~3u);
    a.setGetPosition(0);a.setPutPosition(0);a.setField18(0);a.setBufferOffset(0);
    a.setFlagsByte(static_cast<std::uint8_t>(flags));
    if(capacity && !(flags&8)){a.setMaxGet(0xffffffff);a.terminateBuffer();}
    else a.setMaxGet(0);
    a.installReadCallback();a.installWriteCallback();
}
