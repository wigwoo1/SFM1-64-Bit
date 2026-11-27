#pragma once

#include "source_map_touch_links_candidate.h"
#include "source_map_trigger_io_candidate.h"
#include "source_sliding_door_motion_candidate.h"

#include <map>

enum class SourceMapDoorCloseCandidate { endTouchAll, testAfterEndTouch };

// Map-owned navigation state. Collision callers supply ordered contact pairs,
// physics step durations, and the points at which pending scans and I/O run.
class SourceMapDoorRuntimeCandidate {
    struct Entity {
        SourceMapTouchEntity touch;
        bool client = false;
    };
    struct Trigger {
        uint32_t door;
        SourceMapDoorCloseCandidate close;
        SourceClientMapTriggerCandidate io;
    };
    using Input = SourceMapTriggerInputCandidate;
    using Signal = SourceMapTriggerOutputCandidate;
    std::map<uint32_t, Entity> entities_;
    std::map<uint32_t, Trigger> triggers_;
    std::vector<SourceSlidingDoorMotionCandidate> doors_;
    SourceMapTriggerQueueCandidate queue_;
    SourceMapUntouchQueueCandidate untouchQueue_;

    SourceMapTouchEntity* findTouch(uint32_t handle) {
        const auto entity = entities_.find(handle);
        return entity == entities_.end() ? nullptr : &entity->second.touch;
    }

    void route(uint32_t owner, float now, Signal signal) {
        const auto& trigger = triggers_.at(owner);
        if (signal == Signal::startTouch) {
            queue_.enqueue(now, 0, trigger.door, Input::open);
        } else if (trigger.close == SourceMapDoorCloseCandidate::endTouchAll) {
            if (signal == Signal::endTouchAll) queue_.enqueue(now, 0, trigger.door, Input::close);
        } else if (signal == Signal::endTouch) {
            queue_.enqueue(now, 0, owner, Input::touchTest);
        } else if (signal == Signal::notTouching) {
            queue_.enqueue(now, 0, trigger.door, Input::close);
        }
    }

    auto contactSink(float now) {
        return [this, now](SourceMapTouchEvent event, uint32_t owner, uint32_t peer) {
            if (event != SourceMapTouchEvent::start && event != SourceMapTouchEvent::end) return;
            const auto target = triggers_.find(owner);
            if (target == triggers_.end()) return;
            const auto emit = [this, owner, now](Signal signal) { route(owner, now, signal); };
            if (event == SourceMapTouchEvent::start) {
                target->second.io.startTouch(peer, entities_.at(peer).client, emit);
            } else {
                target->second.io.endTouch(peer, [this](uint32_t handle) {
                    return entities_.find(handle) != entities_.end();
                }, emit);
            }
        };
    }

public:
    void addActor(uint32_t handle, bool client) {
        auto [entry, added] = entities_.try_emplace(handle);
        if (!added) throw std::invalid_argument("duplicate map entity handle");
        entry->second.client = client;
        entry->second.touch.solid = true;
    }

    uint32_t addDoor(const SourceSlidingDoorSpawnCandidate& spawn) {
        const auto index = static_cast<uint32_t>(doors_.size());
        doors_.emplace_back(spawn);
        return index;
    }

    void addTrigger(uint32_t handle, uint32_t door, SourceMapDoorCloseCandidate close) {
        if (door >= doors_.size() || entities_.find(handle) != entities_.end()) {
            throw std::invalid_argument("invalid map door trigger association");
        }
        auto [entry, added] = entities_.try_emplace(handle);
        entry->second.touch.trigger = true;
        try {
            triggers_.emplace(handle, Trigger{door, close, {}});
        } catch (...) {
            entities_.erase(entry);
            throw;
        }
    }

    template<class Schedule>
    void beginScan(uint32_t handle, Schedule schedule) {
        auto& touch = entities_.at(handle).touch;
        sourceBeginTouchScan(touch, handle, [&](SourceMapTouchEvent, uint32_t owner, uint32_t) {
            untouchQueue_.schedule(owner, touch);
            schedule(owner);
        });
    }

    void reportContact(uint32_t solid, uint32_t trigger, float now) {
        sourceReportMapTouch(entities_.at(solid).touch, solid, entities_.at(trigger).touch,
            trigger, false, contactSink(now));
    }

    void finishScan(uint32_t handle, float now) {
        sourceFinishTouchScan(entities_.at(handle).touch, handle,
            [this](uint32_t id) { return findTouch(id); }, contactSink(now));
    }

    // The map simulation invokes this service after entity thinking.
    void servicePendingScans(float now) {
        untouchQueue_.service([this](uint32_t id) { return findTouch(id); },
            [this, now](uint32_t id) { finishScan(id, now); });
    }

    template<class Deliver>
    void service(float now, bool stepping, int32_t& credits, Deliver deliver) {
        queue_.service(now, stepping, credits, [&](const auto& event) {
            if (event.input == Input::touchTest) {
                triggers_.at(event.target).io.touchTest(
                    [&](Signal signal) { route(event.target, now, signal); });
            } else {
                doors_.at(event.target).applyInput(event.input == Input::open
                    ? SourceSlidingDoorInputCandidate::open : SourceSlidingDoorInputCandidate::close);
            }
            deliver(event);
        });
    }

    void advanceUnobstructed(float duration) {
        for (auto& door : doors_) door.advanceUnobstructed(duration);
    }

    const SourceSlidingDoorMotionCandidate& door(uint32_t index) const { return doors_.at(index); }
    size_t touchCount(uint32_t handle) const { return triggers_.at(handle).io.touchCount(); }
    size_t pendingInputs() const { return queue_.pendingCount(); }
};
