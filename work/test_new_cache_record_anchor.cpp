#include "source_light_cache_new_record_anchor_candidate.h"
#include <iostream>
#include <limits>
using Point=std::array<float,3>;
struct CheckedWorld{
 unsigned leafCalls=0,traceCalls=0;bool fail=false;
 int32_t leafAt(const Point&point){if(point!=Point{16,16,64})throw std::runtime_error("wrong candidate");++leafCalls;return 7;}
 uint32_t contents(int32_t){throw std::runtime_error("unexpected contents query");}
 void constrainToStartLeaf(const Point&,float,Point&){throw std::runtime_error("unexpected clipping");}
 SourceLightCacheTraceCandidate trace(const Point&start,const Point&end,uint32_t mask){if(start!=Point{1,2,3}||end!=Point{16,16,64}||mask!=0x4081)throw std::runtime_error("wrong trace");++traceCalls;return {fail?2.f:1.f,false};}
};
int main(){try{
 const Point point{1,2,3};for(int32_t setting:{0,1,2,-1,INT32_MIN}){CheckedWorld world;const auto result=sourceLightCacheNewRecordAnchorCandidate(point,7,setting,world);if(setting==0){if(result!=point||world.leafCalls||world.traceCalls)throw std::runtime_error("zero setting did work or changed point");}else if(result!=Point{16,16,64}||world.leafCalls!=1||world.traceCalls!=1)throw std::runtime_error("nonzero setting did not use cache policy");}
 CheckedWorld world;const Point outside{100000,0,0};if(sourceLightCacheNewRecordAnchorCandidate(outside,7,0,world)!=outside||world.leafCalls||world.traceCalls)throw std::runtime_error("disabled centering applied cache-cell guard");
 unsigned guards=0;auto rejects=[&](auto action){bool failed=false;try{action();}catch(const std::runtime_error&){failed=true;}if(!failed)throw std::runtime_error("bad query accepted");++guards;};
 rejects([&]{sourceLightCacheNewRecordAnchorCandidate({std::numeric_limits<float>::infinity(),0,0},7,0,world);});rejects([&]{sourceLightCacheNewRecordAnchorCandidate(point,-1,0,world);});world.fail=true;rejects([&]{sourceLightCacheNewRecordAnchorCandidate(point,7,1,world);});
 if(point!=Point{1,2,3}||guards!=3)throw std::runtime_error("guard/input preservation");std::cout<<"PASS five integer settings, zero-mode callback bypass/outside-cell-domain copy, three guards and input preservation\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
