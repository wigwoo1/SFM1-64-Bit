#pragma once
#include <cstdint>
// Arena-owned, serialized native functor. Counter is deliberately non-atomic.
struct StudioMaterialZeroArgumentCallCandidate {
 using Method=void(*)(uintptr_t);
 uint32_t references=1;Method method=nullptr;uintptr_t owner=0;
 uint32_t addReference(){return ++references;}
 template<class Destroy>uint32_t releaseReference(Destroy destroy){auto remaining=--references;if(!remaining){destroy(this,0u);return 0;}return remaining;}
 void invoke(){method(owner);}
};
