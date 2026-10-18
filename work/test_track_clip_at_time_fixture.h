#pragma once
#include "track_clip_at_time_candidate.h"
#include <vector>
#include <cassert>
#include <limits>
struct TrackClipTimeFixture {
    struct Clip {std::int32_t start,end;bool eligible;};
    struct Track {unsigned type=3;std::vector<std::uint32_t> handles;};
    Clip clips[4]{{-10,0,true},{0,10,true},{5,20,true},{0,50,false}};
    unsigned prepared=0;bool appendOnPrepare=false;
    auto trackType(Track* t) {return t->type;}
    auto count(Track* t) {return static_cast<std::int32_t>(t->handles.size());}
    void prepareTrack(Track* t) {++prepared;if(appendOnPrepare)t->handles.push_back(2);}
    auto* handles(Track* t) {return t->handles.data();}
    Clip* resolve(std::uint32_t h) {return h>=1 && h<=4?&clips[h-1]:nullptr;}
    bool eligible(Clip* c) {return c->eligible;}
    auto start(Clip* c) {return c->start;}
    auto end(Clip* c) {return c->end;}
};
inline void testTrackClipAtTimeFixture() {
    TrackClipTimeFixture f;TrackClipTimeFixture::Track track{3,{99,4,1,2,3}};
    assert(!findTrackClipAtTimeCandidate(&track,-11,f));
    assert(findTrackClipAtTimeCandidate(&track,-10,f)==&f.clips[0]);
    assert(findTrackClipAtTimeCandidate(&track,0,f)==&f.clips[1]);
    assert(findTrackClipAtTimeCandidate(&track,5,f)==&f.clips[1]); // First eligible overlapping clip wins.
    assert(findTrackClipAtTimeCandidate(&track,10,f)==&f.clips[2]);
    assert(!findTrackClipAtTimeCandidate(&track,20,f));
    const auto prepared=f.prepared;track.type=2;
    assert(!findTrackClipAtTimeCandidate(&track,5,f) && f.prepared==prepared);
    track.type=3;track.handles.clear();
    assert(!findTrackClipAtTimeCandidate(&track,5,f) && f.prepared==prepared);
    track.handles={99};f.appendOnPrepare=true;
    assert(findTrackClipAtTimeCandidate(&track,0,f)==&f.clips[1]); // Capture count after preparation.
    f.appendOnPrepare=false;track.handles={1};
    f.clips[0]={std::numeric_limits<std::int32_t>::min(),std::numeric_limits<std::int32_t>::max(),true};
    assert(findTrackClipAtTimeCandidate(&track,std::numeric_limits<std::int32_t>::min(),f)==&f.clips[0]);
    assert(!findTrackClipAtTimeCandidate(&track,std::numeric_limits<std::int32_t>::max(),f));
}
