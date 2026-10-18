#include "elapsed_frame_clock_candidate.h"
#include <iostream>
#include <limits>
void check(bool ok){if(!ok)throw std::runtime_error("clock test failed");}
int main(){ElapsedFrameClockCandidate c;c.setPackedRate(3u|(1000u<<16));c.reset(100,0);check(c.sample(100,40)==0);check(c.sample(33333433,40)==0);check(c.sample(33333434,40)==1);check(c.sample(1000000100,40)==30);check(c.sample(1300000100,40)==39);check(c.sample(2000000100,40)==20);c.reset(9000000000,17);check(c.sample(9000000000,40)==17);check(c.sample(9100000000,40)==20);c.setPackedRate(3u|(1001u<<16));c.reset(0,0);check(c.sample(1001000000,40)==30);check(c.sample(1000999999,40)==29);auto last=c.sample(std::numeric_limits<uint64_t>::max(),40);check(last<40);bool bad=false;try{c.setPackedRate(0);}catch(...){bad=true;}check(bad);std::cout<<"elapsed clock passed: boundaries, late frames, pause baseline, seek baseline, fractional rate, long duration, invalid rate\n";}
