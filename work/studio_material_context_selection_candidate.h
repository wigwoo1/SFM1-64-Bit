#pragma once
#include <cstdint>
// 1000cb10/1000cb50. Storage callbacks must operate on the calling thread.
// slot0 semantics remain supplied by the context implementation.
template<class Get,class Set,class Slot0>
uintptr_t studioMaterialGetContextCandidate(uintptr_t immediate,Get get,Set set,Slot0 slot0){
 auto current=get();if(!current){current=immediate;set(current);}slot0(current);return current;
}
template<class Get,class Set,class Slot0>
uintptr_t studioMaterialSetContextCandidate(uintptr_t next,Get get,Set set,Slot0 slot0){
 auto previous=get();if(next)slot0(next);set(next);return previous;
}
