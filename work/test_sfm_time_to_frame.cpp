#include "sfm_time_to_frame_candidate.h"
#include "editor_adjust_time_candidate.h"
#include "sfm_frame_to_time_candidate.h"
#include <limits>
#include <iostream>
int main() {
    SfmFrameRateCandidate rate{24,10000};
    assert(sfmTimeToFrameCandidate(10000,rate,0)==24);
    assert(sfmTimeToFrameCandidate(-10000,rate,1)==-24);
    assert(sfmTimeToFrameCandidate(208,rate,0)==0);
    assert(sfmTimeToFrameCandidate(209,rate,0)==1);
    assert(sfmTimeToFrameCandidate(-209,rate,0)==0);
    assert(sfmTimeToFrameCandidate(-210,rate,0)==-1);
    assert(sfmTimeToFrameCandidate(-208,rate,0)==0);
    assert(sfmTimeToFrameCandidate(416,rate,2)==1);
    assert(sfmTimeToFrameCandidate(416,rate,1)==1);
    assert(sfmTimeToFrameCandidate(416,rate,99)==0);
    assert(sfmTimeToFrameCandidate(-416,rate,1)==-1);
    assert(sfmTimeToFrameCandidate(-416,rate,2)==0);
    assert(sfmTimeToFrameCandidate(10010000,{30,10010},0)==30000);
    // Low32 result wrap at an extreme packed rate.
    assert(sfmTimeToFrameCandidate(std::numeric_limits<std::int32_t>::max(),{65535,1},99)==2147418113);
    const std::int32_t positive[]={417,417,417,417,0};
    const std::int32_t negative[]={-417,-417,0,0,0};
    const unsigned modes[]={0,1,2,3,99};
    for(unsigned i=0;i<5;++i) {
        assert(sfmAdjustTimeCandidate(416,rate,modes[i])==positive[i]);
        assert(sfmAdjustTimeCandidate(-416,rate,modes[i])==negative[i]);
    }
    assert(sfmAdjustTimeCandidate(10010000,{30,10010},0)==10010000);
    auto frame=sfmTimeToFrameCandidate(209,rate,0);
    assert(sfmFrameToTimeCandidate(static_cast<float>(frame),rate,0,[](double v,unsigned){return sfmFloorDoubleTimeCandidate(v+0.5);})==417);
    std::cout << "x64 time-to-frame: signed boundaries/literal modes/fractional rate/low32 wrap/composed snap passed\n";
}

