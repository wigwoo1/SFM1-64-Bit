#pragma once
#include <cstdint>
// CDataModel slots60/61, storage byte+C18C bit3. Semantic name unresolved.
inline bool documentDataModelFlagCandidate(uint8_t flags){return (flags&8)!=0;}
inline void setDocumentDataModelFlagCandidate(uint8_t&flags,bool enabled){
 flags=uint8_t((flags&0xf7u)|(enabled?8u:0u));
}
// CDataModel slots119/120 share storage+C18C, using bit4. The native
// setter takes argument bit0, rather than normalizing any nonzero byte.
// A set bit suppresses element change propagation in1035E400.
inline bool documentChangePropagationSuppressedCandidate(uint8_t flags){
 return (flags&0x10u)!=0;
}
inline void setDocumentChangePropagationSuppressedCandidate(uint8_t&flags,uint32_t argument){
 flags=uint8_t((flags&0xefu)|((argument&1u)<<4));
}
// Recovered normal-return sequence of1029ADA0. Caller supplies owned runtime
// flag storage and operations; native exception/unwind behavior is not modeled.
template<class Evaluate>
void documentDataModelScopeCandidate(uint8_t&flags,Evaluate&&evaluate){
 const bool saved=documentDataModelFlagCandidate(flags);
 setDocumentDataModelFlagCandidate(flags,false);
 evaluate();
 setDocumentDataModelFlagCandidate(flags,saved);
}
