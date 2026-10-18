#include "graph_typed_attribute_candidate.h"
#include <cassert>
#include <iostream>
struct Element{};struct Adapter{Element value;unsigned handle=9,filter=1,resolves=0,types=0;bool missing=false,unresolved=false;
 unsigned* findAttribute1035dd50(Element*,const char*){return missing?nullptr:&handle;}unsigned* handleStorage10002780(unsigned* p){return p;}
 Element* resolveModelSlot18(unsigned h){++resolves;assert(h==(missing?0xffffffff:9));return unresolved?nullptr:&value;}
 std::uintptr_t liveTypeSymbol(std::uintptr_t tag){assert(tag==0x10eff4f4);return 0x123456789;}
 unsigned elementTypeSlot4(Element* p,std::uintptr_t s){++types;assert(p==&value && s==0x123456789);return filter;}
};
int main(){static_assert(sizeof(void*)==8);Element owner;GraphAttributeDefaultCandidate state;state.guard=0x80;Adapter a;assert(graphTypedAttributeCandidate(&owner,"graphEditorState",0x10eff4f4,state,a)==&a.value);assert(state.guard==0x81 && state.handle==0xffffffff && state.field14==0xffffffff && !state.field18);
 state.field18=77;a.missing=true;assert(graphTypedAttributeCandidate(&owner,"settings",0x10eff4f4,state,a)==&a.value && state.field18==77);a.filter=256;assert(!graphTypedAttributeCandidate(&owner,"settings",0x10eff4f4,state,a));a.unresolved=true;unsigned before=a.types;assert(!graphTypedAttributeCandidate(&owner,"settings",0x10eff4f4,state,a) && a.types==before);
 std::cout<<"x64 typed attribute: shared lazy defaults, invalid handle delegation, wide symbol and low-byte type filter passed\n";}
