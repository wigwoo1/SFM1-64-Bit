#pragma once
#include "reverse_reference_iterator.h"
#include <memory>
#include <stdexcept>
//1035D9E0 pushes newest token to embedded head.1035DA30 removes first match.
class RecoveredReverseReferenceListCandidate {
 RecoveredReverseReferenceNodeCandidate head{};
public:
 RecoveredReverseReferenceListCandidate()=default;
 RecoveredReverseReferenceListCandidate(const RecoveredReverseReferenceListCandidate&)=delete;
 RecoveredReverseReferenceListCandidate&operator=(const RecoveredReverseReferenceListCandidate&)=delete;
 ~RecoveredReverseReferenceListCandidate(){while(head.next){auto*node=head.next;head.next=node->next;delete node;}}
 RecoveredReverseReferenceNodeCandidate* first(){return firstRecoveredReverseReferenceCandidate(&head);}
 void add(uint32_t attribute){if(attribute==0xffffffffu)throw std::runtime_error("invalid reference attribute token");
  if(head.attribute!=0xffffffffu){auto node=std::make_unique<RecoveredReverseReferenceNodeCandidate>(head);head.next=node.release();}
  head.attribute=attribute;
 }
 void remove(uint32_t attribute){if(head.attribute==attribute){if(head.next){auto*node=head.next;head=*node;delete node;}else head.attribute=0xffffffffu;return;}
  auto*previous=&head;while(previous->next){auto*node=previous->next;if(node->attribute==attribute){previous->next=node->next;delete node;return;}previous=node;}
 }
};
