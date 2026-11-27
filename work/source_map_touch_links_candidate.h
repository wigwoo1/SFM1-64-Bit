#pragma once

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

enum class SourceMapTouchEvent : uint32_t { start = 1, touch, end, schedule };

struct SourceMapTouchLink {
    uint32_t peer, stamp;
    bool notifyEnd;
};

struct SourceMapTouchEntity {
    uint32_t stamp = 0;
    bool pending = false;
    bool solid = false, trigger = false, suppressSolidTouch = false;
    bool deleting = false, markedForRemoval = false;
    uint32_t parent = UINT32_MAX;
    std::vector<SourceMapTouchLink> links;
};

// IDs are generation-bearing handles. Lookup returns null for expired handles.
// The event sink records or queues notifications and must not reenter this tracker.
template<class Emit>
void sourceBeginTouchScan(SourceMapTouchEntity& entity, uint32_t id, Emit emit) {
    ++entity.stamp;
    if (!entity.pending) {
        entity.pending = true;
        emit(SourceMapTouchEvent::schedule, id, id);
    }
}

// The server services a snapshot in insertion order. New relinks can queue
// work for a later pass, and clearing a pending bit does not move an entry.
class SourceMapUntouchQueueCandidate {
    std::vector<uint32_t> queued_;

public:
    void schedule(uint32_t id, const SourceMapTouchEntity& entity) {
        if (!entity.deleting) queued_.push_back(id);
    }
    const std::vector<uint32_t>& queued() const { return queued_; }

    template<class Lookup, class Finish>
    void service(Lookup lookup, Finish finish) {
        auto snapshot = std::exchange(queued_, {});
        for (const auto id : snapshot) {
            const auto* entity = lookup(id);
            if (entity && entity->pending) finish(id);
        }
    }
};

template<class Emit>
void sourceReportMapTouch(SourceMapTouchEntity& first, uint32_t firstId,
    SourceMapTouchEntity& second, uint32_t secondId, bool suppress, Emit emit) {
    if (firstId == secondId || first.parent == secondId || second.parent == firstId ||
        first.deleting || second.deleting || first.markedForRemoval || second.markedForRemoval ||
        (first.trigger && second.trigger && !first.solid && !second.solid)) return;

    const auto mark = [&](SourceMapTouchEntity& owner, uint32_t ownerId,
                          const SourceMapTouchEntity& peer, uint32_t peerId) {
        const auto existing = std::find_if(owner.links.begin(), owner.links.end(),
            [peerId](const auto& link) { return link.peer == peerId; });
        if (existing != owner.links.end()) {
            existing->stamp = owner.stamp;
            if (!suppress) emit(SourceMapTouchEvent::touch, ownerId, peerId);
            return;
        }
        const bool notify = ((owner.solid && !owner.suppressSolidTouch) || owner.trigger) && !peer.trigger;
        owner.links.insert(owner.links.begin(), {peerId, owner.stamp, notify});
        if (notify && !suppress) {
            emit(SourceMapTouchEvent::start, ownerId, peerId);
            emit(SourceMapTouchEvent::touch, ownerId, peerId);
        }
    };
    mark(first, firstId, second, secondId);
    mark(second, secondId, first, firstId);
}

template<class Lookup, class Emit>
void sourceFinishTouchScan(SourceMapTouchEntity& owner, uint32_t id, Lookup lookup, Emit emit) {
    for (auto link = owner.links.begin(); link != owner.links.end();) {
        if (link->stamp == UINT32_MAX) {
            const auto* peer = lookup(link->peer);
            if (peer && !owner.deleting && !peer->deleting) emit(SourceMapTouchEvent::touch, id, link->peer);
            ++link;
            continue;
        }
        if (link->stamp == owner.stamp) {
            ++link;
            continue;
        }
        if (auto* peer = lookup(link->peer)) {
            const auto reverse = std::find_if(peer->links.begin(), peer->links.end(),
                [id](const auto& entry) { return entry.peer == id; });
            if (reverse != peer->links.end()) {
                if (reverse->notifyEnd && lookup(id)) emit(SourceMapTouchEvent::end, link->peer, id);
                peer->links.erase(reverse);
            }
            if (link->notifyEnd) emit(SourceMapTouchEvent::end, id, link->peer);
        }
        link = owner.links.erase(link);
    }
    owner.pending = false;
}
