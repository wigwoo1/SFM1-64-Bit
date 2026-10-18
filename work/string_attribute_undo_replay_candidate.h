#pragma once
#include "string_attribute_undo_payload_candidate.h"
#include <limits>
// 10367690/103676e0. Calls same setter as normal edits; stack-level record
// suppression is the caller's runtime obligation, not guessed here.
template<class Adapter>
inline void replayStringAttributeUndoCandidate(StringAttributeUndoPayloadCandidate& record,bool redo,Adapter& a) {
    auto* owner=a.resolveUndoOwner(record.ownerHandle);
    if(!owner)return;
    const auto name=record.attributeName==std::numeric_limits<std::uintptr_t>::max()?"":reinterpret_cast<const char*>(record.attributeName);
    auto* attribute=a.findUndoAttribute(owner,name);
    if(!attribute || (a.flags(attribute)&0x20))return;
    a.updateUndoAttribute(attribute,redo?&record.newSymbol:&record.oldSymbol);
}
