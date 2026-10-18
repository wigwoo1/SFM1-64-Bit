#include "graph_bookmark_undo_identity_candidate.h"
#include <cassert>
#include <iostream>
struct Owner{std::uint32_t handle;};struct Attribute{Owner* owner;std::uintptr_t name;};
struct Adapter{
 Attribute* attribute;Owner replacement{123};unsigned phase=0;
 std::uint32_t captureModelSlot80(const char*){assert(phase++==0);attribute->owner=&replacement;return 0x80000000;}
 std::uint32_t captureModelSlot81(const char*){assert(phase++==1);attribute->name=0x100000055ull;return 0xffffffff;}
 auto* owner8(Attribute* p){return p->owner;}auto handle1c(Owner* p){return p->handle;}
 auto name14(Attribute* p){return p->name;}
};
int main(){Owner old{7};Attribute attribute{&old,8};Adapter a{&attribute};GraphAttributeUndoIdentityCandidate o;
 const char* label="CUndoAttributeArrayCopyAllElement";graphAttributeUndoIdentityCandidate(o,&attribute,label,a);
 assert(o.captured8==0x80000000&&o.capturedc==0xffffffff&&o.handle18==123&&o.name14==0x100000055ull);
 assert(o.label==label&&o.byte10==0&&o.vtableTag==0x10b37d84);
 std::cout<<"x64 attribute undo identity: capture order, live owner/name, 32-bit handles and wide name pointer passed\n";
}
