#pragma once
#include "native_device_reset_candidate.h"
#include "studio_presentation_builder_candidate.h"
// Hooks retain resource/lifetime ownership; this adapter supplies recovered rebuilds.
template<class Hooks,class Presentation>struct NativeResetPresentationLifecycleCandidateT{
 Hooks& hooks;
 NativeDeviceResetOwnerCandidate owner;
 unsigned& priorFormat;
 unsigned& depthBits;
 Presentation& presentation;
 unsigned descriptorFlags(StudioPresentationDescriptorCandidate& descriptor){return descriptor.flags;}
 void rebuild(HWND window,UINT adapter,StudioPresentationDescriptorCandidate& descriptor,bool initial){
  studioPresentationBuilderCandidate(owner.parameters,owner.flags,depthBits,window,adapter,priorFormat,descriptor,initial,presentation);
 }
 void managerCleanup(){hooks.managerCleanup();}
 void ownerHook(bool enabled){hooks.ownerHook(enabled);}
 auto& commandState(){return hooks.commandState();}
 template<class State>void flush(State& value){hooks.flush(value);}
 template<class State>auto* device(State& value){return hooks.device(value);}
 void resetFailure(HRESULT value){hooks.resetFailure(value);}
 void postReset(bool a,bool b){hooks.postReset(a,b);}
};
