#pragma once
#include <cstdint>
// Single-owner state. Callbacks represent ordinary/alternate draw and final
// binding cleanup; their rendering bodies are outside this recovered component.
struct PendingDrawFlushCandidate {
    bool present=false;
    uint8_t guard=0,pending=0,alternate=0;
    template<class Draw,class Finish> void flush(Draw draw,Finish finish) {
        if(!present||guard||!pending)return;
        guard=1;
        draw(alternate!=0);
        guard=0;pending=0;alternate=0;
        finish();
    }
};
