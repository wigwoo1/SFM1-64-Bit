#pragma once
#include <cstdint>
//105cbcf0: ancestor is evaluated before own byte, with short circuit.
// Parent callback must use recovered children/same-owner/restriction0 selector.
// Nonnull group and acyclic parent chain are original preconditions.
template<class Element,class Parent,class ReadVisible>
std::uint8_t controlGroupVisibleCandidate(Element* group,Parent& parent,
 ReadVisible& readVisible){
 auto* ancestor=parent(group);
 if(ancestor&&controlGroupVisibleCandidate(ancestor,parent,readVisible)==0)return 0;
 return readVisible(group);
}
