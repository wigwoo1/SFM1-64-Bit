#pragma once
#include <cstdint>
// Portable pointer storage: native node is attribute token followed by next pointer.
struct RecoveredReverseReferenceNodeCandidate {uint32_t attribute=0xffffffffu;RecoveredReverseReferenceNodeCandidate*next=nullptr;};
inline RecoveredReverseReferenceNodeCandidate* firstRecoveredReverseReferenceCandidate(RecoveredReverseReferenceNodeCandidate*embeddedHead){return embeddedHead&&embeddedHead->attribute!=0xffffffffu?embeddedHead:nullptr;}
inline RecoveredReverseReferenceNodeCandidate* nextRecoveredReverseReferenceCandidate(RecoveredReverseReferenceNodeCandidate*node){return node?node->next:nullptr;}
