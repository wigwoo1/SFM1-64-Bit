#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

// UNVERIFIED RESEARCH PROTOTYPE: generated table does NOT match the installed
// engine. Do not integrate or use this pool in production. See the saved handoff.
// Attempted implementation of the published shuffle recipe, not an embedded
// original binary table. Reference: ValveSoftware/source-sdk-2013,
// src/tier1/generichash.cpp (MakeRandomValues/Hash12). The MSVCRT generator is
// made explicit so host C library rand() differences cannot change the table.
inline std::array<uint32_t,256> sourcePlaneHashPermutationCandidate(){
 std::array<uint32_t,256> table{};for(uint32_t i=0;i<256;++i)table[i]=i;uint32_t seed=0xdeadbeef;
 for(unsigned pass=0;pass<8;++pass)for(unsigned i=0;i<256;++i){seed=seed*214013u+2531011u;const unsigned other=(seed>>16)&255u;const auto v=table[i];table[i]=table[other];table[other]=v;}return table;
}
inline uint32_t sourcePlaneHashCandidate(const std::array<float,3>& plane){
 static const auto table=sourcePlaneHashPermutationCandidate();std::array<uint32_t,3> words{};std::memcpy(words.data(),plane.data(),12);
 const auto first=words[0];uint32_t even=table[first&255],odd=table[(first>>8)&255];
 even=table[odd^(first>>24)];odd=table[even^((first>>16)&255)];even=table[odd^((first>>8)&255)];odd=table[even^(first&255)];
 for(size_t i=1;i<3;++i){auto word=words[i];even=table[odd^(word>>24)];odd=table[even^((word>>16)&255)];even=table[odd^((word>>8)&255)];odd=table[even^(word&255)];}return (even<<8)|odd;
}
struct SourceDisplacementPlanePoolCandidate {
 using Plane=std::array<float,3>;
 struct Entry{Plane plane;uint32_t id;};
 std::vector<std::vector<Entry>> buckets;std::vector<Plane> planes;bool masked;
 explicit SourceDisplacementPlanePoolCandidate(size_t count,bool useMask):buckets(count),masked(useMask){if(!count||count>65536||(masked&&(count&(count-1))))throw std::runtime_error("invalid plane hash bucket count");}
 uint32_t bucket(const Plane& p)const{auto opposite=p;for(auto& v:opposite)v=-v;const auto h=sourcePlaneHashCandidate(p)^sourcePlaneHashCandidate(opposite);return masked?h&uint32_t(buckets.size()-1):h%uint32_t(buckets.size());}
 uint16_t intern(const Plane& p){for(float v:p)if(!std::isfinite(v))throw std::runtime_error("nonfinite cache plane");auto& list=buckets[bucket(p)];auto opposite=p;for(auto& v:opposite)v=-v;
  for(const auto& e:list){if(e.plane==p)return uint16_t(e.id);if(e.plane==opposite)return uint16_t(e.id|32768u);}
  if(planes.size()>=32767)throw std::runtime_error("displacement plane index capacity exceeded");const auto id=uint32_t(planes.size());list.push_back({p,id});planes.push_back(p);return uint16_t(id);
 }
};
