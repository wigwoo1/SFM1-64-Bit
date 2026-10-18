#pragma once
#include <map>
#include "session_runtime.h"
#include "undo_reference_count.h"
// Explicit native handle-record metadata. Deferred collection is separate.
struct RuntimeUndoRetention{
 struct Record{uint32_t packed=0;uint8_t flags=0;uint32_t fileId=0xffffffff;};
 SessionRuntime&runtime;std::map<uint32_t,Record>records;uint8_t collectionFlags=0;
 explicit RuntimeUndoRetention(SessionRuntime&r):runtime(r){}
 Record*lookup(uint32_t h){if(h==0xffffffff||!runtime.elements.resolve(runtime.graph,h))return nullptr;auto it=records.find(h);return it==records.end()?nullptr:&it->second;}
 void retain(uint32_t h){auto p=lookup(h);if(!p)return;if(!(p->packed&0x3ff00000)&&p->fileId!=0xffffffff)collectionFlags|=0x10;p->flags|=1;p->packed=retainUndoReferenceCount(p->packed);}
 void release(uint32_t h){auto p=lookup(h);if(!p)return;p->packed=releaseUndoReferenceCount(p->packed);if(!(p->packed&0x3ff00000)&&p->fileId!=0xffffffff)collectionFlags|=0x10;}
};
// SessionRuntime graph records are all base elements; unknown/stale handles are
// stored but counter lookup skips them, matching native unresolved assignment.
struct RuntimeUndoRetainedTarget{
 RuntimeUndoRetention&ledger;uint32_t target=0xffffffff;
 explicit RuntimeUndoRetainedTarget(RuntimeUndoRetention&l):ledger(l){}
 RuntimeUndoRetainedTarget(const RuntimeUndoRetainedTarget&)=delete;
 RuntimeUndoRetainedTarget&operator=(const RuntimeUndoRetainedTarget&)=delete;
 RuntimeUndoRetainedTarget(RuntimeUndoRetainedTarget&&other)noexcept:ledger(other.ledger),target(other.target){other.target=0xffffffff;}
 ~RuntimeUndoRetainedTarget(){ledger.release(target);}
 void assign(uint32_t next){if(next==target)return;ledger.release(target);target=next;ledger.retain(target);}
};
