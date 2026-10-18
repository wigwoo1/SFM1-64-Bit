#pragma once
#include "editor_current_time_protocol_candidate.h"
#include "editor_adjust_time_candidate.h"
#include "sfm_frame_to_time_candidate.h"
#include "editor_normal_time_candidate.h"
#include <cmath>
#include <vector>
#include <cassert>
struct EditorCurrentTimeFixture {
    struct Attribute {float rate=29.97f;} attribute;
    struct Object {} object;
    bool frames=false,override=false,missingObject=false,missingAttribute=false;
    std::int32_t index=24;
    EditorNormalTimeStateCandidate normalState;
    EditorCurrentTimeFixture() {normalState.time684=456;}
    std::vector<unsigned> events;
    bool frameMode() {return frames;}
    Object* rateObject() {events.push_back(1);return missingObject?nullptr:&object;}
    Attribute* frameRateAttribute(Object* p) {assert(p==&object);events.push_back(2);return missingAttribute?nullptr:&attribute;}
    float attributeFloat(Attribute* p) {assert(p==&attribute);events.push_back(3);return p->rate;}
    auto constructRate(float rate) {events.push_back(4);return constructSfmFrameRateCandidate(rate,
        [](float value){return static_cast<std::int32_t>(std::nearbyint(value));});}
    auto frameIndex() {events.push_back(5);return index;}
    std::int32_t convertFrame(float frame,SfmFrameRateCandidate rate,unsigned mode) {
        events.push_back(6);assert(mode==0);
        return sfmFrameToTimeCandidate(frame,rate,mode,[](double value,unsigned rounding){
            return sfmDoubleTimeConvertCandidate(value,rounding,[](double){assert(false);return 0;});});
    }
    bool overrideEnabled() {events.push_back(7);return override;}
    std::int32_t overrideTime() {events.push_back(8);return -123;}
    std::int32_t normalTime() {events.push_back(9);return editorNormalTimeCandidate(normalState,*this);}
    bool engineReady() {events.push_back(10);return false;}
    std::int32_t clockTime() {assert(false);return 0;}
    std::int32_t adjustTime(std::int32_t time,int mode) {assert(mode==0);events.push_back(11);return editorAdjustTimeCandidate(time,static_cast<unsigned>(mode),*this);}
};
inline void testEditorCurrentTimeFixture() {
    EditorCurrentTimeFixture f;
    assert(editorCurrentTimeCandidate(f)==456 && (f.events==std::vector<unsigned>{7,9,10}));
    f.events.clear();f.normalState.flag6c0=true;
    assert(editorCurrentTimeCandidate(f)==334 && (f.events==std::vector<unsigned>{7,9,10,11,1,2,3,4}));
    f.events.clear();f.attribute.rate=24;f.normalState.time684=209;
    assert(editorCurrentTimeCandidate(f)==417);
    f.events.clear();f.normalState.time684=-209;
    assert(editorCurrentTimeCandidate(f)==0);
    f.events.clear();f.normalState.time684=-210;
    assert(editorCurrentTimeCandidate(f)==-417);
    f.attribute.rate=29.97f;
    f.events.clear();f.override=true;
    assert(editorCurrentTimeCandidate(f)==-123 && (f.events==std::vector<unsigned>{7,8}));
    f.events.clear();f.frames=true;f.index=30000;
    assert(editorCurrentTimeCandidate(f)==10010000 && (f.events==std::vector<unsigned>{1,2,3,4,5,6}));
    f.events.clear();f.missingObject=true;f.index=24;
    assert(editorCurrentTimeCandidate(f)==10000 && (f.events==std::vector<unsigned>{1,4,5,6}));
    f.events.clear();f.missingObject=false;f.missingAttribute=true;f.index=-1;
    assert(editorCurrentTimeCandidate(f)==-417 && (f.events==std::vector<unsigned>{1,2,4,5,6}));
    f.events.clear();f.missingAttribute=false;f.attribute.rate=24;f.index=24;
    assert(editorCurrentTimeCandidate(f)==10000);
}
