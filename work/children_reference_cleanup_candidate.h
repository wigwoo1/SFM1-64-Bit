#pragma once
#include <cstdint>
// Owner eligibility checked before empty range. Dynamic callbacks preserve
// original re-reads after owner notification. Valid range is caller precondition.
template<class OwnerEligible,class NotifyOwner,class ReadHandle,class RemoveReference>
void childrenReferenceCleanupCandidate(std::int32_t first,std::int32_t last,
    bool notifyFlag,OwnerEligible eligible,NotifyOwner notify,
    ReadHandle readHandle,RemoveReference removeReference) {
    if (notifyFlag && eligible()) notify(first,last);
    for (auto index=first;index<=last;++index) removeReference(readHandle(index));
}
