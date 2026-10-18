#pragma once
#include <cstdint>
template<class Attribute,class Data>struct GraphBookmarkAssignmentFacadeCandidate{std::uintptr_t vtableTag;Attribute* attribute;Data* data;};
template<class Attribute,class Data>GraphBookmarkAssignmentFacadeCandidate(std::uintptr_t,Attribute*,Data*)->GraphBookmarkAssignmentFacadeCandidate<Attribute,Data>;
template<class Target,class Source,class Adapter>void graphBookmarkAssignmentWrapperCandidate(Target* target,Source* source,int count,Adapter& a){
 auto* attribute=a.attribute14(target);auto* data=a.attributeData4(attribute);
 GraphBookmarkAssignmentFacadeCandidate facade{std::uintptr_t{0x10b381bc},attribute,data};
 a.assign1038cb80(facade,source,count);
}
