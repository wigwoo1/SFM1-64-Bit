#pragma once
#include "buffer_string_extract_candidate.h"
// Service identities remain opaque. This candidate requires an explicit adapter.
template<class Adapter>
void cleanupControllerAutosaveCandidate(Adapter& a){
    std::uint8_t path[260];
    a.formatLocatorPath(path,260,a.locatorPathService(),".sfm_autosave.loc");
    auto buffer=a.createBuffer(0,260,1);
    if(a.readSubinterfaceSlot14(path,nullptr,buffer,260,0,0)){
        a.mainInterfaceSlot18(path,nullptr);
        std::uint8_t target[260];
        extractBufferStringCandidate(target,260,buffer);
        a.mainInterfaceSlot18(target,nullptr);
    }
    if(bufferSigned32Candidate(buffer.growSize())>=0){
        if(auto* storage=buffer.sourceData())a.freeBufferStorage(storage);
    }
}
