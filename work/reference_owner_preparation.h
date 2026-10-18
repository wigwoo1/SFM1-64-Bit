#pragma once
#include <cstdint>
//1035E3A0: move all matching nodes from owner+48 to owner+4C, reversing
// their encounter order; append owner handle to cache queue even with no match.
template<class Node,class Key,class Next,class Queue>
void prepareRecoveredReferenceOwnerCandidate(Node*& source,Node*& destination,uintptr_t attribute,bool blocked,Key key,Next next,Queue queue){
 if(blocked)return;Node** link=&source;
 while(auto* node=*link){if(key(node)==attribute){*link=next(node);next(node)=destination;destination=node;}else link=&next(node);}
 queue();
}
