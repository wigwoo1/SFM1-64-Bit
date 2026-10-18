#pragma once
#include "editor_map_fallback_protocol_candidate.h"
#include <vector>
#include <cassert>
struct EditorMapFallbackFixture {
    struct Object {unsigned id;};Object primary{1},trackObject{2},clip{3};
    bool missingPrimary=false,missingSecond=false,missingTrack=false,missingClip=false,firstEligible=true,secondEligible=true;
    unsigned resolves=0,begins=0,typeChecks=0;
    std::vector<unsigned> events;
    Object* resolvePrimary() {events.push_back(1);++resolves;return missingPrimary || (missingSecond && resolves==2)?nullptr:&primary;}
    unsigned beginModeScope() {events.push_back(10+(++begins));return begins;}
    void restoreModeScope(unsigned s) {events.push_back(20+s);}
    int currentTime() {events.push_back(2);return 123;}
    Object* track(Object* p) {assert(p==&primary);events.push_back(3);return missingTrack?nullptr:&trackObject;}
    int mapTime(Object* p,int t,int mode) {assert(p==&primary && t==123 && mode==0);events.push_back(4);return 456;}
    Object* findClip(Object* p,int t) {assert(p==&trackObject && t==456);events.push_back(5);return missingClip?nullptr:&clip;}
    bool eligible(Object* p) {assert(p==&clip);events.push_back(6);return ++typeChecks==1?firstEligible:secondEligible;}
};
inline void testEditorMapFallbackFixture() {
    EditorMapFallbackFixture f;
    assert(resolveEditorMapFallbackCandidate(f)==&f.clip);
    assert((f.events==std::vector<unsigned>{1,11,1,2,12,3,4,5,6,22,6,21}));
    EditorMapFallbackFixture lost;lost.secondEligible=false;
    assert(!resolveEditorMapFallbackCandidate(lost) && lost.typeChecks==2 && lost.events.back()==21);
    EditorMapFallbackFixture rejected;rejected.firstEligible=false;
    assert(!resolveEditorMapFallbackCandidate(rejected) && rejected.typeChecks==1);
    for(unsigned missing=0;missing<4;++missing) {
        EditorMapFallbackFixture absent;
        absent.missingPrimary=missing==0;absent.missingSecond=missing==1;
        absent.missingTrack=missing==2;absent.missingClip=missing==3;
        assert(!resolveEditorMapFallbackCandidate(absent));
        if(missing==0)assert(absent.begins==0 && absent.events.size()==1);
        else assert(absent.begins==2 && absent.events[absent.events.size()-2]==22 && absent.events.back()==21);
    }
}
