#pragma once
#include "recovered_frame_ticks_candidate.h"
#include "session_tick_grid_candidate.h"
inline std::vector<int32_t> sessionFrameTickGridCandidate(int32_t start,uint32_t packedRate,size_t count){if(!count||count>10000)throw std::runtime_error("frame grid count");std::vector<int32_t>ticks;ticks.reserve(count);for(size_t frame=0;frame<count;++frame){auto offset=recoveredFrameTicksCandidate(float(frame),packedRate,0);int64_t tick=int64_t(start)+offset;if(tick<INT32_MIN||tick>INT32_MAX)throw std::runtime_error("frame grid overflow");ticks.push_back(int32_t(tick));}return ticks;}
