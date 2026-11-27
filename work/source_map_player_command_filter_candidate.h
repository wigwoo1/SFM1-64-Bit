#pragma once
#include <array>
#include <cstdint>

struct SourceMapPlayerCommandFilterInputCandidate {
    std::array<float,3> view{},movement{};
    uint32_t buttons=0;
    uint8_t impulse=0;
};

// Only the installed player's command filtering and forwarding boundary.
// Host owns player state, cvars and the actual movement implementation.
template<class Host>
void sourceMapFilterPlayerCommandCandidate(SourceMapPlayerCommandFilterInputCandidate& command,Host& host) {
    host.resetCommandFlag();
    if(host.inputAccumulatorIsZero()) host.cacheView(command.view);
    if(host.suppressedByPlayerFlag() || (!host.optionEnabled() && !host.globalMode() && host.serverTime()<3.f)) {
        command.movement={}; command.buttons=0; command.impulse=0; command.view=host.cachedView();
    } else if(host.playerOptionFlag()) {
        if(host.secondaryOptionEnabled()) {
            host.clearPlayerOptionFlag(); host.requestSecondaryOptionZero();
        } else command.buttons|=4;
    }
    host.runMovement(command);
}
