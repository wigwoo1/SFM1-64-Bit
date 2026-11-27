#pragma once

#include <cmath>
#include <optional>

// SFM stores an integer rate or a broadcast rate with the same nominal
// integer and a 1.001 time base. The document supplies a finite positive rate.
inline float sourceMapFrameSyncRate(float documentRate) {
    const float nominal = std::nearbyint(documentRate);
    if (std::abs(documentRate - nominal) < 0.001f) return nominal;
    const float broadcast = (documentRate * 1001.f) * 0.001f;
    const float broadcastNominal = std::nearbyint(broadcast);
    if (std::abs(broadcast - broadcastNominal) >= 0.01f) return nominal;
    return static_cast<float>(double(broadcastNominal) * 10000.0 / 10010.0);
}

enum class SourceMapFrameSyncNotice { enabled, disabled, changedRate };

template<class Host>
void sourceMapApplyFrameSync(bool requested, bool suspended, Host& host) {
    if (suspended || !host.hasSyncVariable() || !host.hasRateVariable()) return;
    const float rate = sourceMapFrameSyncRate(host.documentRate().value_or(24.f));
    if (host.syncEnabled() != requested) {
        host.setSyncEnabled(requested);
        host.setFrameRate(requested ? rate : 0.f);
        if (!requested) {
            host.notice(SourceMapFrameSyncNotice::disabled, 0.f);
            return;
        }
        host.notice(SourceMapFrameSyncNotice::enabled, rate);
    }
    if (requested && host.frameRate() != rate) {
        host.notice(SourceMapFrameSyncNotice::changedRate, rate);
        host.setFrameRate(rate);
    }
}
