#include "source_displacement_face_input_candidate.h"
#include <functional>
#include <iostream>
#include <limits>
int main(){try{
 std::vector<SourceDisplacementFaceInputCandidate>faces(4);auto label=[&](size_t i,int16_t v){std::memcpy(faces[i].data()+12,&v,2);};label(0,0);label(1,-1);label(2,9);label(3,0);
 if(sourceDisplacementFaceMapCandidate(faces,2)!=std::vector<uint16_t>{3,0xffff})throw std::runtime_error("last writer/missing/ignored face");
 std::vector<std::array<float,3>>vertices{{1,2,3},{4,5,6},{7,8,9},{10,11,12}};std::vector<std::array<uint16_t,2>>edges{{0,3},{0,1},{2,0},{0,3}};std::vector<int32_t>surfedges{0,-1,2,-3};SourceDisplacementFaceInputCandidate f{};int16_t count=4;std::memcpy(f.data()+8,&count,2);
 const auto quad=sourceDisplacementFaceQuadCandidate(f,vertices,edges,surfedges);for(size_t i=0;i<4;++i)if(quad[i]!=vertices[i])throw std::runtime_error("signed surfedge endpoint selection");
 unsigned rejected=0;auto reject=[&](std::function<void()>run){try{run();}catch(const std::runtime_error&){++rejected;return;}throw std::runtime_error("invalid data accepted");};
 label(0,-2);reject([&]{sourceDisplacementFaceMapCandidate(faces,2);});reject([&]{sourceDisplacementFaceMapCandidate({},65536);});
 count=3;std::memcpy(f.data()+8,&count,2);reject([&]{sourceDisplacementFaceQuadCandidate(f,vertices,edges,surfedges);});count=4;std::memcpy(f.data()+8,&count,2);
 int32_t first=-1;std::memcpy(f.data()+4,&first,4);reject([&]{sourceDisplacementFaceQuadCandidate(f,vertices,edges,surfedges);});first=1;std::memcpy(f.data()+4,&first,4);reject([&]{sourceDisplacementFaceQuadCandidate(f,vertices,edges,surfedges);});first=0;std::memcpy(f.data()+4,&first,4);
 surfedges[0]=INT32_MIN;reject([&]{sourceDisplacementFaceQuadCandidate(f,vertices,edges,surfedges);});surfedges[0]=0;edges[0][0]=65535;reject([&]{sourceDisplacementFaceQuadCandidate(f,vertices,edges,surfedges);});edges[0][0]=0;vertices[0][0]=std::numeric_limits<float>::infinity();reject([&]{sourceDisplacementFaceQuadCandidate(f,vertices,edges,surfedges);});
 if(rejected!=8)throw std::runtime_error("guard coverage");std::cout<<"PASS last matching face, ignored/missing labels, signed endpoints and eight invalid-input guards\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
