#include "source_spatial_point_partition_candidate.h"
#include <future>
#include <iostream>
int main(){try{
    static_assert(sizeof(void*)==8,"owned x64");unsigned checks=0;auto require=[&](bool ok){if(!ok)throw std::runtime_error("partition safety assertion");++checks;};
    std::vector<SourceSpatialPointEntryCandidate>entries;for(unsigned i=0;i<12;++i)entries.push_back({(uint64_t(i+1)<<32)|0xdecafbadu,0x65,{-4,-4,-4,4,4,4}});SourceSpatialPointPartitionCandidate partition(entries);const auto baseline=partition.queryPoint({0,0,0},{0,0,0},0x65);require(baseline.size()==12);for(unsigned i=0;i<12;++i)require(baseline[i]==entries[11-i].entityId);require(partition.queryPoint({0,0,0},{0,0,0},0).empty());require(partition.queryPoint({0,0,0},{0,0,0},0x65,5)==std::vector<uint64_t>(baseline.begin(),baseline.begin()+5));
    auto reject=[&](auto&&fn){bool caught=false;try{fn();}catch(const std::exception&){caught=true;}require(caught);require(partition.queryPoint({0,0,0},{0,0,0},0x65)==baseline);};
    {auto bad=entries;bad[0].entityId=0;reject([&](){partition=SourceSpatialPointPartitionCandidate(bad);});}
    {auto bad=entries;bad[1].entityId=bad[0].entityId;reject([&](){partition=SourceSpatialPointPartitionCandidate(bad);});}
    {auto bad=entries;bad[0].mask=0x10000;reject([&](){partition=SourceSpatialPointPartitionCandidate(bad);});}
    {auto bad=entries;bad[0].bounds[0]=10;reject([&](){partition=SourceSpatialPointPartitionCandidate(bad);});}
    for(unsigned j=0;j<6;++j){auto bad=entries;bad[0].bounds[j]=std::numeric_limits<float>::quiet_NaN();reject([&](){partition=SourceSpatialPointPartitionCandidate(bad);});}
    for(unsigned j=0;j<3;++j)for(float bad:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity()}){std::array<float,3>p{};p[j]=bad;reject([&](){partition.queryPoint(p,{1,2,3},1);});reject([&](){partition.queryPoint({0,0,0},p,1);});}
    reject([&](){partition.queryPoint({0,0,0},{1,2,3},0x10000);});reject([&](){partition.storedBounds(12);});
    {std::vector<SourceSpatialPointEntryCandidate>bad(65536);reject([&](){SourceSpatialPointPartitionCandidate tooMany(bad);});}
    // Caller buffers may disappear: the partition owns every ID/box/list.
    entries.clear();entries.shrink_to_fit();require(partition.queryPoint({-100,0,0},{200,0,0},0x65)==baseline);
    const auto&shared=partition;std::array<std::future<bool>,4>jobs;for(auto&job:jobs)job=std::async(std::launch::async,[&](){for(unsigned i=0;i<128;++i)if(shared.queryPoint({-100,0,0},{200,0,0},0x65)!=baseline)return false;return true;});for(auto&job:jobs)require(job.get());
    auto retained=partition.queryPoint({0,0,0},{0,0,0},0x65);partition=SourceSpatialPointPartitionCandidate({});require(retained==baseline&&partition.queryPoint({0,0,0},{0,0,0},0x65).empty());
    std::cout<<"PASS owned partition safety checks="<<checks<<" concurrent_queries=512\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
