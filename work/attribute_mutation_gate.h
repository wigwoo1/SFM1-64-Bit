#pragma once
#include <cstdint>
// Native 0x103623B0. Phase meanings remain unnamed until independently recovered.
inline bool attributeMutationPermitted(uint16_t flags,int phase){
 if(flags&0x20)return false;
 return phase==0||phase==1||phase==2||(phase==4&&!(flags&0x400));
}
template<class PrepareOwner,class MarkOwner> bool beginAttributeMutation(uint16_t&flags,int phase,PrepareOwner prepare,MarkOwner mark){
 if(!attributeMutationPermitted(flags,phase))return false;
 if(!(flags&0x80)&&(flags&0x2000))prepare();
 flags|=0x4080;mark();return true;
}
