#include "graph_current_selection_candidate.h"
#include <cassert>
#include <iostream>
struct Child{unsigned handle=7;};struct Element{};
struct Adapter{Child child;Element one,two,out;bool hasChild=true,firstMissing=false,attributeMissing=false,secondMissing=false;unsigned resolves=0,attributes=0;
 Child* globalChild344(){return hasChild?&child:nullptr;}unsigned childHandle80(Child* c){return c->handle;}
 Element* resolveModelSlot18(unsigned h){if(++resolves==1){assert(h==7);child.handle=9;return firstMissing?nullptr:&one;}assert(h==9);return secondMissing?nullptr:&two;}
 Element* nullSelection(){return nullptr;}Element* lookup10002830(Element* e){++attributes;assert(e==(secondMissing?nullptr:&two));return attributeMissing?nullptr:&one;}
 Element* lookup102c4420(Element* e){++attributes;assert(e==&one);return &out;}
};
int main(){static_assert(sizeof(void*)==8);Adapter a;assert(graphCurrentSelectionCandidate(a)==&a.out && a.resolves==2 && a.attributes==2);Adapter b;b.hasChild=false;assert(!graphCurrentSelectionCandidate(b) && !b.resolves);Adapter c;c.firstMissing=true;assert(!graphCurrentSelectionCandidate(c) && c.resolves==1 && !c.attributes);Adapter d;d.attributeMissing=true;assert(!graphCurrentSelectionCandidate(d) && d.attributes==1);Adapter e;e.secondMissing=true;assert(graphCurrentSelectionCandidate(e)==&e.out && e.attributes==2);std::cout<<"x64 current selection: null gates, live handle reload, second-result delegation and native pointer chain passed\n";}
