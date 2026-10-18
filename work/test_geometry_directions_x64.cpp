#include "model_skinning_geometry.h"
#include <fstream>
#include <iostream>
std::vector<uint8_t> read(const char*p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("fixture missing");return {(std::istreambuf_iterator<char>(f)),{}};}
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");if(argc!=3)throw std::runtime_error("MDL VVD required");
 std::array<uint8_t,48>v{};auto put=[&](size_t o,float x){std::memcpy(v.data()+o,&x,4);};put(0,1);v[15]=1;put(16,2);put(28,1);put(40,-.5f);put(44,1.5f);
 std::array<uint8_t,16>t{};std::array<float,4>raw{0,1,0,-1};std::memcpy(t.data(),raw.data(),16);
 GeometryMatrixCandidate rotation{0,-1,0,10,1,0,0,20,0,0,1,30};auto a=skinGeometryVertexCandidate(v,&t,{rotation});
 if(a.position!=std::array<float,3>{10,22,30}||a.normal!=std::array<float,3>{0,1,0}||a.tangent!=std::array<float,4>{-1,0,0,-1}||a.uv!=std::array<float,2>{-.5f,1.5f}||!a.hasTangent)throw std::runtime_error("rotation attributes");
 if(skinGeometryVertexCandidate(v,nullptr,{rotation}).hasTangent)throw std::runtime_error("absent tangent");
 auto mdl=read(argv[1]),vvd=read(argv[2]);auto bones=readGeometrySkeletonCandidate(mdl);uint32_t checksum;std::memcpy(&checksum,mdl.data()+8,4);auto geometry=readVvdGeometryCandidate(vvd,checksum,0);std::vector<GeometryMatrixCandidate>palette;for(auto&b:bones)palette.push_back(b.skin);float error=0;for(size_t n=0;n<geometry.vertices.size();++n){auto&vertex=geometry.vertices[n];auto out=skinGeometryVertexCandidate(vertex,&geometry.tangents.at(n),palette);for(size_t k=0;k<3;++k){float normal,tangent;std::memcpy(&normal,vertex.data()+28+k*4,4);std::memcpy(&tangent,geometry.tangents[n].data()+k*4,4);error=std::max(error,std::abs(out.normal[k]-normal));error=std::max(error,std::abs(out.tangent[k]-tangent));}if(std::memcmp(out.uv.data(),vertex.data()+40,8)||std::memcmp(&out.tangent[3],geometry.tangents[n].data()+12,4))throw std::runtime_error("UV/handedness changed");}
 if(geometry.vertices.size()!=47850||error>0.0001f)throw std::runtime_error("bind directions");std::cout<<"x64 rotation/UV/handedness passed vertices="<<geometry.vertices.size()<<" bind_direction_error="<<error<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
