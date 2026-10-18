#pragma once
#include <cstdint>
// Position-present descriptor branch100219e0; other descriptor branches separate.
template<class Copy>void studioMaterialPositionVertexUploadCandidate(uintptr_t destination,uintptr_t source,uint32_t returnedStride,uint32_t bytes,Copy copy){if(source&&returnedStride&&destination)copy(destination,source,bytes);}
