#pragma once
#include <cstdint>
struct GraphUndoObjectCandidate{std::uintptr_t vtableTag;const char* label;std::uint32_t captured8,capturedc;unsigned char byte10;void* owner14;};
template<class Adapter>GraphUndoObjectCandidate* constructGraphUndoCandidate(GraphUndoObjectCandidate* object,void* owner,const char* label,Adapter& a){
 object->vtableTag=0x10a8aa24;object->captured8=object->capturedc=0xffffffff;
 object->captured8=a.captureModelSlot80(label);object->capturedc=a.captureModelSlot81(label);
 object->label=label;object->byte10=0;object->vtableTag=0x10aad97c;object->owner14=owner;return object;
}
