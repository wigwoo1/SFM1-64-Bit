#include "model_skinning_geometry.h"
#include <fstream>
#include <iostream>
std::vector<uint8_t> read(const char*path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("fixture missing");return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),{});}
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");if(argc!=3)throw std::runtime_error("MDL VVD required");
 GeometryMatrixCandidate identity{1,0,0,0,0,1,0,0,0,0,1,0},translated=identity;translated[3]=4;translated[7]=8;translated[11]=12;
 std::array<uint8_t,48>vertex{};auto put=[&](size_t o,float v){std::memcpy(vertex.data()+o,&v,4);};put(0,.25f);put(4,.75f);vertex[12]=0;vertex[13]=1;vertex[15]=2;put(16,1);put(20,2);put(24,3);
 if(skinGeometryPositionCandidate(vertex,{identity,translated})!=std::array<float,3>{4,8,12})throw std::runtime_error("weighted translation");
 vertex[13]=2;bool rejected=false;try{skinGeometryPositionCandidate(vertex,{identity,translated});}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("bad bone accepted");
 auto mdl=read(argv[1]),vvd=read(argv[2]);auto bones=readGeometrySkeletonCandidate(mdl);uint32_t checksum;std::memcpy(&checksum,mdl.data()+8,4);auto geometry=readVvdGeometryCandidate(vvd,checksum,0);std::vector<GeometryMatrixCandidate>palette;for(auto&b:bones){if(b.procedure)throw std::runtime_error("procedural fixture unsupported");palette.push_back(b.skin);}float maxError=0;for(auto&v:geometry.vertices){auto p=skinGeometryPositionCandidate(v,palette);for(size_t r=0;r<3;++r){float original;std::memcpy(&original,v.data()+16+r*4,4);maxError=std::max(maxError,std::abs(p[r]-original));}}
 if(bones.size()!=92||geometry.vertices.size()!=47850||maxError>0.0001f)throw std::runtime_error("installed bind mismatch");
 uint32_t boneStart;std::memcpy(&boneStart,mdl.data()+160,4);unsigned malformed=0;
 for(auto edit:std::vector<std::pair<size_t,uint32_t>>{{156,0xffffffff},{160,0xffffffff},{size_t(boneStart)+4,0},{size_t(boneStart),0x7fffffff},{size_t(boneStart)+44,0x7fc00000}}){auto changed=mdl;std::memcpy(changed.data()+edit.first,&edit.second,4);bool failed=false;try{readGeometrySkeletonCandidate(changed);}catch(const std::runtime_error&){failed=true;}if(!failed)throw std::runtime_error("malformed skeleton accepted");++malformed;}
 std::cout<<"x64 weighted translation/bone rejection passed bones="<<bones.size()<<" vertices="<<geometry.vertices.size()<<" bind_position_error="<<maxError<<" malformed_skeleton_rejections="<<malformed<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
