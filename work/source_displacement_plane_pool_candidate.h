#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

// Pure hash/pool mechanics with explicit permutation data. There is deliberately
// no random-generator default and no embedded Valve constant table. Reference
// data provenance/distribution is a host responsibility, distinct from algorithm
// verification. The oracle reads its private fixture outside the workspace.
using SourcePlaneHashPermutationCandidate=std::array<uint8_t,256>;
inline uint32_t sourcePlaneHashCandidate(const std::array<float,3>& plane,const SourcePlaneHashPermutationCandidate& table){
 std::array<uint32_t,3> words{};std::memcpy(words.data(),plane.data(),12);
 const auto first=words[0];uint32_t even=table[first&255],odd=table[(first>>8)&255];
 even=table[odd^(first>>24)];odd=table[even^((first>>16)&255)];even=table[odd^((first>>8)&255)];odd=table[even^(first&255)];
 for(size_t i=1;i<3;++i){auto word=words[i];even=table[odd^(word>>24)];odd=table[even^((word>>16)&255)];even=table[odd^((word>>8)&255)];odd=table[even^(word&255)];}return (even<<8)|odd;
}
struct SourceDisplacementPlanePoolCandidate {
 using Plane=std::array<float,3>;
 struct Entry{Plane plane;uint32_t id;};
 std::vector<std::vector<Entry>> buckets;std::vector<Plane> planes;bool masked;SourcePlaneHashPermutationCandidate permutation;
 explicit SourceDisplacementPlanePoolCandidate(size_t count,bool useMask,const SourcePlaneHashPermutationCandidate& table):buckets(count),masked(useMask),permutation(table){std::array<bool,256> seen{};for(auto v:table){if(seen[v])throw std::runtime_error("hash table is not a permutation");seen[v]=true;}if(!count||count>65536||(masked&&(count&(count-1))))throw std::runtime_error("invalid plane hash bucket count");}
 uint32_t bucket(const Plane& p)const{auto opposite=p;for(auto& v:opposite)v=-v;const auto h=sourcePlaneHashCandidate(p,permutation)^sourcePlaneHashCandidate(opposite,permutation);return masked?h&uint32_t(buckets.size()-1):h%uint32_t(buckets.size());}
 uint16_t intern(const Plane& p){for(float v:p)if(!std::isfinite(v))throw std::runtime_error("nonfinite cache plane");auto& list=buckets[bucket(p)];auto opposite=p;for(auto& v:opposite)v=-v;
  for(const auto& e:list){if(e.plane==p)return uint16_t(e.id);if(e.plane==opposite)return uint16_t(e.id|32768u);}
  if(planes.size()>=32767)throw std::runtime_error("displacement plane index capacity exceeded");const auto id=uint32_t(planes.size());list.push_back({p,id});planes.push_back(p);return uint16_t(id);
 }
};
