#pragma once
#include <cstdint>
// Pointer-aware counterpart of the earlier fixed-endpoint flush fixture.
// Metadata is a caller-owned opaque pointer passed only to alternate draw.
// Single-owner callbacks must keep endpoint/metadata valid. After draw, native
// code rereads the endpoint before cleanup; a replacement is therefore visible.
struct PendingDrawDispatchCandidate {
 uintptr_t mesh=0,metadata=0;uint8_t guard=0,pending=0,alternate=0;
 template<class Ops>void flush(Ops&ops){
  const auto target=mesh;if(!target||guard||!pending)return;
  const bool other=alternate!=0;guard=1;
  ops.draw(target,other,other?metadata:0);
  guard=0;pending=0;alternate=0;
  ops.finish(mesh);
 }
};
