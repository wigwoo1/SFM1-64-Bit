#pragma once

#include "source_map_simulation_clock_candidate.h"

#include <algorithm>
#include <deque>
#include <vector>

enum class SourceMapTriggerOutputCandidate {
    startTouch, startTouchAll, endTouch, endTouchAll, touching, notTouching,
};

// A client-only trigger with no named filter. The collision system supplies
// touch notifications. Handles include the entity serial to recognize reuse.
class SourceClientMapTriggerCandidate {
    std::vector<uint32_t> touching_;
    bool disabled_ = false;

public:
    void setDisabled(bool disabled) { disabled_ = disabled; }
    size_t touchCount() const { return touching_.size(); }

    template<typename Emit>
    void startTouch(uint32_t handle, bool client, Emit emit) {
        if (!client) return;
        const bool added = std::find(touching_.begin(), touching_.end(), handle) == touching_.end();
        if (added) touching_.push_back(handle);
        emit(SourceMapTriggerOutputCandidate::startTouch);
        if (added && touching_.size() == 1) emit(SourceMapTriggerOutputCandidate::startTouchAll);
    }

    template<typename Alive, typename Emit>
    void endTouch(uint32_t handle, Alive alive, Emit emit) {
        if (!alive(handle)) return;
        const auto found = std::find(touching_.begin(), touching_.end(), handle);
        if (found == touching_.end()) return;
        touching_.erase(found);
        emit(SourceMapTriggerOutputCandidate::endTouch);
        touching_.erase(std::remove_if(touching_.begin(), touching_.end(),
            [&](uint32_t other) { return !alive(other); }), touching_.end());
        if (touching_.empty()) emit(SourceMapTriggerOutputCandidate::endTouchAll);
    }

    template<typename Emit>
    void touchTest(Emit emit) const {
        if (!disabled_) {
            emit(touching_.empty() ? SourceMapTriggerOutputCandidate::notTouching
                                   : SourceMapTriggerOutputCandidate::touching);
        }
    }
};

enum class SourceMapTriggerInputCandidate { open, close, touchTest };

struct SourceMapTriggerEventCandidate {
    float fireTime;
    uint32_t target;
    SourceMapTriggerInputCandidate input;
};

// Map inputs have nonnegative delays and can enqueue additional inputs when
// dispatched. Inputs at the same time keep their insertion order.
class SourceMapTriggerQueueCandidate {
    std::deque<SourceMapTriggerEventCandidate> pending_;

public:
    size_t pendingCount() const { return pending_.size(); }

    void enqueue(float now, float delay, uint32_t target, SourceMapTriggerInputCandidate input) {
        const float due = now + delay;
        if (delay < 0.f || !std::isfinite(now) || !std::isfinite(due)) {
            throw std::invalid_argument("invalid map event time");
        }
        const auto next = std::upper_bound(pending_.begin(), pending_.end(), due,
            [](float time, const SourceMapTriggerEventCandidate& event) { return time < event.fireTime; });
        pending_.insert(next, {due, target, input});
    }

    template<typename Deliver>
    void service(float now, bool stepping, int32_t& credits, Deliver deliver) {
        while (!pending_.empty() && sourceMapEventDueCandidate(now, pending_.front().fireTime, stepping, credits)) {
            const auto event = pending_.front();
            pending_.pop_front();
            deliver(event);
            if (stepping) --credits;
        }
    }
};
