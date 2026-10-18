#include "source_phy_point_trace_candidate.h"
#include <fstream>
#include <future>
#include <iostream>
#include <iterator>
#include <limits>
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"owned x64");if(argc!=2)throw std::runtime_error("PHY required");std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("PHY read");const std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});const SourcePhyPointTraceCandidate shape(b);const auto baseline=shape.bounds({128,-256,512},{23,47,99});unsigned checks=0;
    auto reject=[&](const std::array<float,3>&origin,const std::array<float,3>&angles){bool caught=false;try{shape.bounds(origin,angles);}catch(const std::exception&){caught=true;}if(!caught)throw std::runtime_error("invalid bounds request accepted");if(shape.bounds({128,-256,512},{23,47,99})!=baseline)throw std::runtime_error("failed request changed shape");++checks;};
    for(unsigned j=0;j<3;++j){for(float bad:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity()}){std::array<float,3>p{1,2,3};p[j]=bad;reject(p,{0,0,0});reject({0,0,0},p);}for(float bad:{36001.f,-36001.f}){std::array<float,3>a{0,0,0};a[j]=bad;reject({0,0,0},a);}}
    std::array<std::future<bool>,4>jobs;for(auto&job:jobs)job=std::async(std::launch::async,[&](){for(unsigned i=0;i<32;++i)if(shape.bounds({128,-256,512},{23,47,99})!=baseline)return false;return true;});for(auto&job:jobs){if(!job.get())throw std::runtime_error("shared const shape bounds changed across threads");++checks;}
    // Bounds computation must not change existing collision answers.
    const auto trace=shape.tracePoint({-100,0,0},{200,0,0},{0,0,0},{0,0,0},1);shape.bounds({-123,234,-345},{-10,20,-30});const auto after=shape.tracePoint({-100,0,0},{200,0,0},{0,0,0},{0,0,0},1);if(trace.start!=after.start||trace.end!=after.end||trace.fraction!=after.fraction||trace.plane.normal!=after.plane.normal||trace.contents!=after.contents)throw std::runtime_error("bounds changed trace");++checks;
    SourcePhyCompactGeometryCandidate geometry(b,true);const auto metadata=sourcePhysicsSupportMetadataTableCandidate(geometry.solids.at(0));std::array<unsigned,4>modes{};for(const auto&m:metadata){const auto flags=m.payload[2];if(flags&1)++modes[0];if(flags&2)++modes[1];if(flags&4)++modes[2];}if(metadata.empty())modes[3]=unsigned(geometry.solids[0].size());std::cout<<"PASS bounds safety checks="<<checks<<" metadata_seeded="<<modes[0]<<" metadata_span="<<modes[1]<<" metadata_runs="<<modes[2]<<" no_metadata="<<modes[3]<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
