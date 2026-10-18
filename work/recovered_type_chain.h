#pragma once
#include <cstdint>
// Recovered flat three-level native type chain: concrete, immediate base,
// element, plus the invalid symbol sentinel accepted as element.
struct RecoveredTypeChain{uint32_t concrete,base,element;};
inline bool recoveredTypeAccepts(const RecoveredTypeChain&chain,uint32_t requested){return requested==chain.concrete||requested==chain.base||requested==chain.element||requested==0xffffffff;}
inline int recoveredTypeDepth(const RecoveredTypeChain&chain,uint32_t requested){
 if(requested==chain.concrete)return 0;
 if(requested==chain.base)return 1;
 if(requested==chain.element||requested==0xffffffff)return 2;
 return -1;
}
