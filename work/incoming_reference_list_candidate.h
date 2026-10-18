#pragma once
#include <cstdint>
struct IncomingReferenceLinkCandidate {
    std::uint32_t token;
    IncomingReferenceLinkCandidate* next;
};
struct IncomingReferenceListCandidate {
    std::uint32_t headToken = 0xffffffffu;
    IncomingReferenceLinkCandidate* next = nullptr;
};
// Allocation and token callbacks must preserve original side effects and order.
// Original allocation path assumes success; no recovery behavior is inferred.
template<class Allocate, class GetToken>
void addIncomingReferenceCandidate(IncomingReferenceListCandidate& list,
    Allocate allocate, GetToken getToken) {
    if (list.headToken != 0xffffffffu) {
        auto link = allocate();
        link->token = list.headToken;
        link->next = list.next;
        list.next = link;
    }
    list.headToken = getToken();
}
template<class GetToken, class Free>
void removeIncomingReferenceCandidate(IncomingReferenceListCandidate& list,
    GetToken getToken, Free freeLink) {
    const auto token = getToken();
    if (list.headToken == token) {
        auto link = list.next;
        if (!link) { list.headToken = 0xffffffffu; return; }
        list.headToken = link->token;
        list.next = link->next;
        freeLink(link);
        return;
    }
    auto cursor = &list.next;
    while (*cursor) {
        auto link = *cursor;
        if (link->token == token) {
            *cursor = link->next;
            freeLink(link);
            return;
        }
        cursor = &link->next;
    }
}
