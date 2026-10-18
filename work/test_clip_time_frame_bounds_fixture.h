#pragma once
#include "clip_time_frame_bounds_candidate.h"
#include "track_clip_at_time_candidate.h"
#include "seconds_to_sfm_time_candidate.h"
#include "sfm_frame_rate_candidate.h"
#include "sfm_frame_to_time_candidate.h"
#include <cmath>
#include <cassert>
#include <limits>
struct ClipTimeBoundsFixture {
    struct Frame {std::int32_t start,duration;};Frame frame{10,5};
    struct Clip {std::uint32_t frame;};Clip clip{1};
    struct Track {std::uint32_t handle=1;} track;
    unsigned frameResolutions=0;
    auto frameHandle(Clip* c) {return c->frame;}
    Frame* resolveFrame(std::uint32_t h) {++frameResolutions;return h==1?&frame:nullptr;}
    auto frameStart(Frame* f) {return f->start;}
    auto frameDuration(Frame* f) {return f->duration;}
    unsigned trackType(Track*) {return 3;}
    int count(Track*) {return 1;}
    void prepareTrack(Track*) {}
    auto* handles(Track* t) {return &t->handle;}
    Clip* resolve(std::uint32_t h) {return h==1?&clip:nullptr;}
    bool eligible(Clip*) {return true;}
    auto start(Clip* c) {return clipStartTimeCandidate(c,*this);}
    auto end(Clip* c) {return clipEndTimeCandidate(c,*this);}
};
inline void testClipTimeFrameBoundsFixture() {
    auto roundDefault=[](float v){return static_cast<std::int32_t>(std::nearbyint(v));};
    struct RateCase {float input;unsigned numerator,units;};
    for(auto c:{RateCase{24.0f,24,10000},RateCase{30.0f,30,10000},RateCase{23.976f,24,10010},
        RateCase{29.97f,30,10010},RateCase{59.94f,60,10010},RateCase{23.5f,24,10000},
        RateCase{24.5f,24,10000},RateCase{25.5f,26,10000},RateCase{0.0f,0,10000},RateCase{70000.0f,4464,10000}}) {
        const auto result=constructSfmFrameRateCandidate(c.input,roundDefault);
        assert(result.numerator==c.numerator && result.timeUnits==c.units);
    }
    auto convert=[](double v,unsigned mode) {return sfmDoubleTimeConvertCandidate(v,mode,
        [](double input){return _mm_cvttsd_si32(_mm_set_sd(std::ceil(input)));});};
    assert(sfmFrameToTimeCandidate(24.0f,{24,10000},0,convert)==10000);
    const std::int32_t positive[]{417,416,417,416},negative[]{-417,-417,-416,-416};
    for(unsigned mode=0;mode<4;++mode) {
        assert(sfmFrameToTimeCandidate(1.0f,{24,10000},mode,convert)==positive[mode]);
        assert(sfmFrameToTimeCandidate(-1.0f,{24,10000},mode,convert)==negative[mode]);
    }
    assert(sfmFrameToTimeCandidate(30000.0f,{30,10010},0,convert)==10010000);
    assert(convert(-0.5,0)==0 && convert(0.5,0)==1);
    assert(convert(-1.75,99)==-1);
    assert(sfmFrameToTimeCandidate(1.0f,{0,10000},0,convert)==std::numeric_limits<std::int32_t>::min());
    assert(secondsToSfmTimeCandidate(0.0f)==0);
    assert(secondsToSfmTimeCandidate(1.0f)==10000);
    assert(secondsToSfmTimeCandidate(-1.0f)==-10000);
    assert(secondsToSfmTimeCandidate(0.00005f)==1);
    assert(secondsToSfmTimeCandidate(-0.00005f)==0);
    assert(secondsToSfmTimeCandidate(-0.03125f)==-312); // Exact binary half-tick after scaling.
    assert(secondsToSfmTimeCandidate(1.0f/24.0f)==417);
    assert(secondsToSfmTimeCandidate(std::numeric_limits<float>::infinity())==std::numeric_limits<std::int32_t>::min());
    assert(secondsToSfmTimeCandidate(std::numeric_limits<float>::quiet_NaN())==std::numeric_limits<std::int32_t>::min());
    ClipTimeBoundsFixture f;
    assert(f.start(&f.clip)==10 && f.end(&f.clip)==15 && f.frameResolutions==2);
    assert(findTrackClipAtTimeCandidate(&f.track,10,f)==&f.clip);
    assert(!findTrackClipAtTimeCandidate(&f.track,15,f));
    f.clip.frame=99;assert(f.start(&f.clip)==0 && f.end(&f.clip)==0);
    assert(!findTrackClipAtTimeCandidate(&f.track,0,f));
    f.clip.frame=1;f.frame={-20,5};
    assert(f.end(&f.clip)==-15 && findTrackClipAtTimeCandidate(&f.track,-20,f)==&f.clip);
    f.frame={std::numeric_limits<std::int32_t>::max(),1};
    assert(f.end(&f.clip)==std::numeric_limits<std::int32_t>::min());
    assert(!findTrackClipAtTimeCandidate(&f.track,std::numeric_limits<std::int32_t>::max(),f));
    f.frame={std::numeric_limits<std::int32_t>::min(),-1};
    assert(f.end(&f.clip)==std::numeric_limits<std::int32_t>::max());
    assert(findTrackClipAtTimeCandidate(&f.track,0,f)==&f.clip);
    f.frame={10,-5};assert(!findTrackClipAtTimeCandidate(&f.track,10,f));
}
