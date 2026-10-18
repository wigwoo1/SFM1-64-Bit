#include "graph_bookmark_assignment_wrapper_candidate.h"
#include <cassert>
#include <iostream>
struct Data{};struct Attribute{Data* data;};struct Target{Attribute* attribute;};
struct Adapter{Attribute* expected;Data* snapshot;int* source;unsigned calls=0;
 Attribute* attribute14(Target* t){return t->attribute;}Data* attributeData4(Attribute* a){return a->data;}
 void assign1038cb80(GraphBookmarkAssignmentFacadeCandidate<Attribute,Data>& f,int* s,int n){assert(f.vtableTag==0x10b381bc && f.attribute==expected && f.data==snapshot && s==source && n==3);++calls;}
};
int main(){static_assert(sizeof(void*)==8);Data data;Attribute attribute{&data};Target target{&attribute};int source[3]={7,8,9};Adapter a{&attribute,&data,source};graphBookmarkAssignmentWrapperCandidate(&target,source,3,a);assert(a.calls==1);std::cout<<"x64 bookmark assignment wrapper: attribute/data facade and source pointer/count forwarding passed\n";}
