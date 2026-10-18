#pragma once
#include <cstdint>
struct StudioMaterialContextBindingsCandidate{uintptr_t material=0,immediate=0,firstHelper=0,secondHelper=0;};
//100212b0. Helpers/buffers remain supplied, preserving actual cleanup order.
template<class Free,class Destroy,class ReleaseBuffer,class ReleaseShared>void studioMaterialContextTeardownCandidate(StudioMaterialContextBindingsCandidate& bindings,Free free,Destroy destroy,ReleaseBuffer releaseBuffer,ReleaseShared releaseShared){
 if(!bindings.immediate)return;
 free(bindings.firstHelper);if(bindings.secondHelper)destroy(bindings.secondHelper,1u);
 bindings.material=bindings.immediate=bindings.firstHelper=bindings.secondHelper=0;
 releaseBuffer(0);releaseBuffer(1);releaseShared();
}
template<class Release>void studioMaterialSharedBuffersReleaseCandidate(uint32_t& users,Release release){if(--users==0){release(0);release(1);}}
