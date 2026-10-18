#include "graph_attribute_access_candidate.h"
#include <cassert>
#include <iostream>
struct Attribute{Attribute* next;std::uintptr_t symbol;unsigned type,handle;};struct Element{Attribute* head;};
struct Adapter{Element* owner;Attribute* installed;unsigned convertible=0,conversions=0;
 std::uintptr_t internModelSlot50(const char*){owner->head=installed;return 0x123456789;}
 Attribute* head30(Element* e){return e->head;}Attribute* next0(Attribute* a){return a->next;}std::uintptr_t symbol14(Attribute* a){return a->symbol;}
 unsigned type10(Attribute* a){return a->type;}unsigned* data4(Attribute* a){return &a->handle;}
 unsigned canConvert10362e50(Attribute*){return convertible;}int convert103630a0(Attribute*,unsigned* out){++conversions;*out=42;return -1;}
};
int main(){static_assert(sizeof(void*)==8);Attribute second{nullptr,0x123456789,0xe1,9},first{&second,2,2,3};Element e{nullptr};Adapter a{&e,&first};assert(graphFindAttributeCandidate(&e,"settings",a)==&second);GraphAttributeDefaultCandidate state;unsigned scratch=88;assert(graphAttributeHandleStorageCandidate(&second,state,scratch,a)==&second.handle);a.convertible=256;assert(graphAttributeHandleStorageCandidate(&first,state,scratch,a)==&state.handle && state.handle==0xffffffff && !a.conversions);a.convertible=1;assert(graphAttributeHandleStorageCandidate(&first,state,scratch,a)==&scratch && scratch==42 && a.conversions==1);std::cout<<"x64 attribute access: live post-intern head, wide symbol, type mask, defaults and shared conversion scratch passed\n";}
