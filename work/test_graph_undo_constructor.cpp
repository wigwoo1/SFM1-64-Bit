#include "graph_undo_constructor_candidate.h"
#include <cassert>
#include <iostream>
struct Adapter{GraphUndoObjectCandidate* object;const char* expected;unsigned binding=1;
 unsigned captureModelSlot80(const char* label){assert(label==expected && object->vtableTag==0x10a8aa24 && object->captured8==0xffffffff && object->capturedc==0xffffffff);binding=2;return 0x12345678;}
 unsigned captureModelSlot81(const char* label){assert(label==expected && binding==2 && object->captured8==0x12345678 && object->capturedc==0xffffffff);return 0x87654321;}
};
int main(){static_assert(sizeof(void*)==8);GraphUndoObjectCandidate object{};const char label[]="fixture";auto* owner=reinterpret_cast<void*>(std::uintptr_t{0x123456789});Adapter a{&object,label};assert(constructGraphUndoCandidate(&object,owner,label,a)==&object && object.owner14==owner && object.label==label && object.capturedc==0x87654321 && object.vtableTag==0x10aad97c && !object.byte10);std::cout<<"x64 graph undo constructor: base/derived publication, live captures, wide owner and label identity passed\n";}
