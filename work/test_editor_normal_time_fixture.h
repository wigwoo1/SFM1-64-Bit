#pragma once
#include "editor_normal_time_candidate.h"
#include "editor_clock_time_candidate.h"
#include <vector>
#include <cassert>
struct EditorNormalTimeFixture {
    bool ready=false;
    std::int32_t clock=100;
    std::vector<unsigned> calls;
    bool engineReady() {calls.push_back(1);return ready;}
    std::int32_t clockTime() {calls.push_back(2);return clock;}
    std::int32_t adjustTime(std::int32_t value,int mode) {assert(mode==0);calls.push_back(3);return value+10;}
};
inline void testEditorNormalTimeFixture() {
    float cached=0;
    assert(editorClockTimeCandidate(cached,1.0f)==10000);
    assert(editorClockTimeCandidate(cached,0.5f)==10000 && cached==1.0f);
    assert(editorClockTimeCandidate(cached,-1.0f)==10000);
    assert(editorClockTimeCandidate(cached,std::numeric_limits<float>::quiet_NaN())==10000);
    assert(editorClockTimeCandidate(cached,2.0f)==20000 && cached==2.0f);
    float cache2=0;
    struct ClockAdapter {
        float& cache;float sample;
        bool engineReady() {return true;}
        auto clockTime() {return editorClockTimeCandidate(cache,sample);}
        std::int32_t adjustTime(std::int32_t,int) {assert(false);return 0;}
    } clockAdapter{cache2,1.0f};
    EditorNormalTimeStateCandidate clockState;clockState.field47c=1;clockState.time430=5000;
    assert(editorNormalTimeCandidate(clockState,clockAdapter)==5000);
    clockAdapter.sample=0.25f;
    assert(editorNormalTimeCandidate(clockState,clockAdapter)==5000);
    clockAdapter.sample=1.5f;
    assert(editorNormalTimeCandidate(clockState,clockAdapter)==10000);
    EditorNormalTimeStateCandidate s;s.time684=456;
    EditorNormalTimeFixture f;
    assert(editorNormalTimeCandidate(s,f)==456 && (f.calls==std::vector<unsigned>{1}));
    for(unsigned flags=0;flags<16;++flags) {
        f.calls.clear();s.flag69c=bool(flags&1);s.flag228=bool(flags&2);s.flag2a8=bool(flags&4);s.flag6c0=bool(flags&8);
        assert(editorNormalTimeCandidate(s,f)==(flags==8?466:456));
        assert(f.calls.size()==(flags==8?2u:1u));
    }
    s={};s.time684=456;f.ready=true;
    for(unsigned index=0;index<4;++index) {
        s.modes480[index]=2;f.calls.clear();
        assert(editorNormalTimeCandidate(s,f)==456 && f.calls.size()==1); // Missing offset uses stored.
        s.time430=25;assert(editorNormalTimeCandidate(s,f)==75);
        s.modes480[index]=0;s.time430=std::numeric_limits<std::int32_t>::min();
    }
    s.field47c=1;s.time430=-1;f.clock=std::numeric_limits<std::int32_t>::max();
    assert(editorNormalTimeCandidate(s,f)==std::numeric_limits<std::int32_t>::min());
    f.ready=false;s.flag6c0=true;
    assert(editorNormalTimeCandidate(s,f)==466); // Engine gate prevents active-clock branch.
}
