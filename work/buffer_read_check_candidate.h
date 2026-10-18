#pragma once
#include <cstdint>
#include <cstring>
inline std::int32_t bufferSigned32Candidate(std::uint32_t value){std::int32_t result;std::memcpy(&result,&value,4);return result;}
template<class Adapter>
bool checkBufferReadCandidate(std::uint32_t request,Adapter& a){
    if(a.errorByte()&2)return false;
    const auto get=a.getPosition();
    if(bufferSigned32Candidate(a.maxGet())<bufferSigned32Candidate(get+request)){
        a.setErrorByte(static_cast<std::uint8_t>(a.errorByte()|2));return false;
    }
    const auto offset=a.bufferOffset();
    if(bufferSigned32Candidate(get)>=bufferSigned32Candidate(offset) &&
       bufferSigned32Candidate(a.capacity())>=bufferSigned32Candidate(get-offset+request))return true;
    if(a.readOverflow(request))return true;
    a.setErrorByte(static_cast<std::uint8_t>(a.errorByte()|2));return false;
}
