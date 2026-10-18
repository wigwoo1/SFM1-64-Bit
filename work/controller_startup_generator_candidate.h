#pragma once
#include <cstdint>
template<class Adapter>std::uint32_t nextControllerStartupRandomCandidate(Adapter& a){
 const std::uint32_t next=a.randomState()*std::uint32_t{0x41c64e6d}+std::uint32_t{0x3039};
 a.setRandomState(next);return (next<<16)|(next>>16);
}
template<class Adapter>std::uint32_t seedControllerStartupRandomCandidate(Adapter& a){
 a.setRandomState(a.seedHelper(0));nextControllerStartupRandomCandidate(a);nextControllerStartupRandomCandidate(a);return nextControllerStartupRandomCandidate(a);
}
