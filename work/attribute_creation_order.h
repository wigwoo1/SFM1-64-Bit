#pragma once
#include <cstdint>
// Native103614F0 prepends newly constructed attributes.
// Caller owns node lifetime and supplies allocation/notifications separately.
template<class AttributeNode>
void prependRecoveredCreatedAttributeCandidate(AttributeNode*&head,AttributeNode*created,uint8_t&elementState){
 created->next=head;head=created;elementState=uint8_t(elementState|3);
}
