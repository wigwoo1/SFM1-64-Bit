#pragma once
#include <cstdint>
struct GraphAttributeUndoIdentityCandidate{
 std::uintptr_t vtableTag=0;const char* label=nullptr;std::uint32_t captured8=0,capturedc=0;
 unsigned char byte10=0;std::uintptr_t name14=0;std::uint32_t handle18=0;
};
template<class Attribute,class Adapter>void graphAttributeUndoIdentityCandidate(GraphAttributeUndoIdentityCandidate& o,Attribute* attribute,const char* label,Adapter& a){
 o.vtableTag=0x10a8aa24;o.captured8=o.capturedc=0xffffffffu;
 o.captured8=a.captureModelSlot80(label);o.capturedc=a.captureModelSlot81(label);
 o.label=label;o.byte10=0;o.vtableTag=0x10b37d84;o.name14=~std::uintptr_t(0);
 auto* owner=a.owner8(attribute);o.handle18=a.handle1c(owner);o.name14=a.name14(attribute);
}
