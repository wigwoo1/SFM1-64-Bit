#include "graph_bookmark_undo_constructor_candidate.h"
#include <cassert>
#include <memory>
#include <vector>
#include <iostream>
struct Owner{unsigned handle=19;};struct Vector{std::vector<unsigned> values{1,2,3};};
struct Attribute{Owner owner;Vector vector;std::uintptr_t name=0x100000044ull;};
struct Adapter{
 Attribute& attribute;unsigned calls=0;std::vector<std::unique_ptr<unsigned[]>> buffers;
 const char* copyAllLabel(){return "CUndoAttributeArrayCopyAllElement";}
 unsigned captureModelSlot80(const char*){return 7;}unsigned captureModelSlot81(const char*){return 8;}
 auto* owner8(Attribute* p){return &p->owner;}unsigned handle1c(Owner* p){return p->handle;}
 auto name14(Attribute* p){return p->name;}unsigned type10(Attribute*){return 21;}
 auto* data4(Attribute* p){return &p->vector;}
 auto* allocate1025ea80(unsigned bytes){
  if(++calls==1)attribute.vector.values={0xffffffffu,0x80000000u};
  buffers.push_back(std::make_unique<unsigned[]>(bytes/4));return buffers.back().get();
 }
 int count0c(Vector* p){assert(calls==1);return int(p->values.size());}
 auto* base0(Vector* p){return p->values.data();}
};
int main(){Attribute attribute;Adapter a{attribute};unsigned source[]{4,5,6};GraphBookmarkUndoCandidate o;
 graphBookmarkUndoConstructorCandidate(o,&attribute,source,3,0x123,a);
 assert(o.nextCount28==3&&o.oldCount20==2&&o.flag2c==0x23);
 assert((std::vector<unsigned>(o.next24,o.next24+3)==std::vector<unsigned>{4,5,6}));
 assert((std::vector<unsigned>(o.old1c,o.old1c+2)==std::vector<unsigned>{0xffffffffu,0x80000000u}));
 assert(o.identity.handle18==19&&o.identity.name14==0x100000044ull&&o.identity.vtableTag==0x10b38f80);
 source[0]=99;attribute.vector.values[0]=0;assert(o.next24[0]==4&&o.old1c[0]==0xffffffffu);
 std::cout<<"x64 composed bookmark undo constructor: identity, snapshot order, callback mutation and owned values passed\n";
}
