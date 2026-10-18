#pragma once
#include <cstdint>
// Attribute owns token and storage; external flag200 prevents typedstorage release.
// References enumeration provides scalaronehandle or arrayascendinghandles.
template<class Attribute,class References,class Detach,class Retire,class ReleaseStorage,class ReleaseAttribute>
void destroyRecoveredReferenceAttributeCandidate(Attribute* attribute,References references,Detach detach,Retire retire,ReleaseStorage releaseStorage,ReleaseAttribute releaseAttribute){
 if(!attribute||(attribute->flags&31)==0)return;
 auto type=attribute->flags&31;if(type==1||type==15)references(*attribute,[&](uint32_t target){detach(target,*attribute);});
 retire(attribute->token);attribute->token=0xffffffffu;
 if(attribute->storage&&!(attribute->flags&0x200)){releaseStorage(type,attribute->storage);attribute->storage=0;}
 releaseAttribute(attribute);
}
