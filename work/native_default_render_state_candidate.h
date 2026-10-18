#pragma once
#include <d3d9.h>
#include "studio_default_render_state_owner_candidate.h"
// Device and queue-enabled fields borrow live storage; synchronization is external.
template<class Device,class Lifecycle>struct NativeDefaultRenderStateServicesCandidateT{
 Device*& currentDevice;uint32_t& queued;Lifecycle& lifecycle;
 unsigned suppressed(){return lifecycle.suppressed();}
 bool queueEnabled(){return queued!=0;}
 void directState(unsigned key,unsigned value){currentDevice->SetRenderState(static_cast<D3DRENDERSTATETYPE>(key),value);}
 void flush(){lifecycle.flush();}
 void publish(uint32_t* block){lifecycle.publish(block);}
 uint32_t* acquire(unsigned header){return lifecycle.acquire(header);}
 auto capability(){return lifecycle.capability();}
 template<class Capability>unsigned flags(Capability* value){return lifecycle.flags(value);}
 template<class Capability>unsigned vendor(Capability* value){return lifecycle.vendor(value);}
};
