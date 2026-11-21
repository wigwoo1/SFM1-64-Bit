#pragma once

#include "source_sliding_door_spawn_candidate.h"

#include <algorithm>

enum class SourceSlidingDoorStateCandidate { open, closed, opening, closing };
enum class SourceSlidingDoorInputCandidate { open, close };

// Parentless toggle door, zero entity rotation, and unobstructed movement.
// The map owner supplies physics steps and delivers the resolved map inputs.
class SourceSlidingDoorMotionCandidate {
    using Point = std::array<float, 3>;
    const SourceSlidingDoorSpawnCandidate spawn_;
    Point origin_;
    Point destination_{};
    Point velocity_{};
    SourceSlidingDoorStateCandidate state_;
    float localTime_ = 0.f;
    float moveDoneTime_ = -1.f;
    bool locked_;

    void finish() {
        origin_ = destination_;
        velocity_ = {};
        moveDoneTime_ = -1.f;
        state_ = state_ == SourceSlidingDoorStateCandidate::opening
            ? SourceSlidingDoorStateCandidate::open : SourceSlidingDoorStateCandidate::closed;
    }

public:
    explicit SourceSlidingDoorMotionCandidate(const SourceSlidingDoorSpawnCandidate& spawn)
        : spawn_(spawn), origin_(spawn.initialOrigin),
          state_(spawn.initiallyOpen ? SourceSlidingDoorStateCandidate::open
                                    : SourceSlidingDoorStateCandidate::closed),
          locked_(spawn.startsLocked) {
        if (!std::isfinite(spawn.speed) || spawn.speed <= 0.f) {
            throw std::invalid_argument("sliding door needs positive speed");
        }
    }

    bool applyInput(SourceSlidingDoorInputCandidate input) {
        using State = SourceSlidingDoorStateCandidate;
        const bool opening = input == SourceSlidingDoorInputCandidate::open;
        if (opening ? (locked_ || state_ == State::open || state_ == State::opening)
                    : state_ == State::closed) {
            return false;
        }
        const Point& target = opening ? spawn_.openOrigin : spawn_.closedOrigin;
        Point velocity{};
        float deadline = -1.f;
        const bool alreadyThere = target == origin_;
        if (!alreadyThere) {
            const Point delta{target[0] - origin_[0], target[1] - origin_[1], target[2] - origin_[2]};
            const float distance = std::sqrt((delta[1] * delta[1] + delta[0] * delta[0]) + delta[2] * delta[2]);
            const float travelTime = distance / spawn_.speed;
            deadline = localTime_ + travelTime;
            const float inverseTime = 1.f / travelTime;
            for (size_t axis = 0; axis < 3; ++axis) velocity[axis] = delta[axis] * inverseTime;
            if (!std::isfinite(deadline) || !std::isfinite(inverseTime)) {
                throw std::runtime_error("unrepresentable sliding door motion");
            }
        }
        state_ = opening ? State::opening : State::closing;
        destination_ = target;
        moveDoneTime_ = deadline;
        if (velocity != velocity_) velocity_ = velocity;
        if (alreadyThere) finish();
        return true;
    }

    void advanceUnobstructed(float frameTime) {
        if (!std::isfinite(frameTime) || frameTime < 0.f) {
            throw std::invalid_argument("invalid sliding door physics step");
        }
        if (moveDoneTime_ < 0.f) return;
        const float moveTime = std::min(frameTime, moveDoneTime_ - localTime_);
        if (moveTime > 0.f) {
            for (size_t axis = 0; axis < 3; ++axis) {
                origin_[axis] += velocity_[axis] * moveTime;
            }
            localTime_ += moveTime;
        }
        if (moveDoneTime_ > 0.f && localTime_ >= moveDoneTime_) finish();
    }

    void setLocked(bool locked) { locked_ = locked; }
    const Point& origin() const { return origin_; }
    const Point& destination() const { return destination_; }
    const Point& velocity() const { return velocity_; }
    SourceSlidingDoorStateCandidate state() const { return state_; }
    float localTime() const { return localTime_; }
    float moveDoneTime() const { return moveDoneTime_; }
};
