#pragma once
#include <cstdint>
#include <vector>
// Initial cached command list, installed VertexLitGeneric mapping ONLY:
// vertexLit=1, flashlightMode=0, info[A4]=-1. Does not execute GPU commands,
// manage the cache, or cover later static/dynamic rendering.
inline std::vector<uint32_t> vertexlitGenericInitialCommandsCandidate(
 bool normalTexture,bool bumpTexture,bool colorEnabled,bool materialMode,
 uint32_t currentSecondaryFlags){
 std::vector<uint32_t> words;
 if(normalTexture||bumpTexture)words={7,5,5,13};
 words.push_back(6);
 if(!colorEnabled){words.push_back(17);words.push_back(1);}
 else{words.push_back(materialMode?13u:11u);words.push_back(1);
  words.insert(words.end(),3,0x3f800000u);}
 if(currentSecondaryFlags&0x40000u)words.push_back(3);
 if(currentSecondaryFlags&2u)words.push_back(4);
 words.push_back(0);
 return words;
}
