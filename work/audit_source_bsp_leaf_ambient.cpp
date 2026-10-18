#include "source_bsp_leaf_ambient_candidate.h"
#include "read_binary_asset_candidate.h"
#include <iostream>
#include <map>
#include <algorithm>
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("map path required");const auto bytes=readBinaryAssetCandidate(argv[1]);
 for(auto encoding:{SourceBspAmbientEncodingCandidate::Hdr,SourceBspAmbientEncodingCandidate::Ldr}){
  const SourceBspLeafAmbientCandidate data(bytes,encoding);size_t direct=0,redirects=0,empty=0,black=0;double maximum=0;std::map<unsigned,size_t>counts;
  for(const auto&leaf:data.leaves){++counts[leaf.count];if(leaf.count)++direct;else if(leaf.sampleOwner)++redirects;else ++empty;}
  for(const auto&sample:data.samples){bool zero=true;for(const auto&face:sample.cube){auto value=SourceBspLeafAmbientCandidate::rgbExpVector(face);for(double channel:value){maximum=std::max(maximum,channel);if(channel!=0)zero=false;}}if(zero)++black;}
  std::cout<<(encoding==SourceBspAmbientEncodingCandidate::Hdr?"HDR":"LDR")<<" leaves="<<data.leaves.size()<<" samples="<<data.samples.size()<<" direct_leaves="<<direct<<" redirected_leaves="<<redirects<<" empty_leaves="<<empty<<" black_cubes="<<black<<" maximum_sdk_vector_channel="<<maximum<<'\n';
  std::cout<<"sample_count_histogram";for(auto entry:counts)std::cout<<' '<<entry.first<<':'<<entry.second;std::cout<<'\n';
 }
 std::cout<<"PASS: read-only ambient datasets validated; native interpolation/shader integration UNVERIFIED\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
