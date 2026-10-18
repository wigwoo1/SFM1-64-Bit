from pathlib import Path
p=Path('work/test_sfm_time_to_frame.cpp')
s=p.read_text().replace('#include "sfm_time_to_frame_candidate.h"','#include "sfm_time_to_frame_candidate.h"\n#include "editor_adjust_time_candidate.h"')
s=s.replace('    auto frame=', '''    const std::int32_t positive[]={417,417,417,417,0};
    const std::int32_t negative[]={-417,-417,0,0,0};
    const unsigned modes[]={0,1,2,3,99};
    for(unsigned i=0;i<5;++i) {
        assert(sfmAdjustTimeCandidate(416,rate,modes[i])==positive[i]);
        assert(sfmAdjustTimeCandidate(-416,rate,modes[i])==negative[i]);
    }
    assert(sfmAdjustTimeCandidate(10010000,{30,10010},0)==10010000);
    auto frame=''')
p.write_text(s)
