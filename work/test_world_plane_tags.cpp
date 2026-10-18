#include "source_brush_line_dispatch_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("map required");unsigned checks=0;auto require=[&](bool x){if(!x)throw std::runtime_error("plane tag assertion");++checks;};
 const std::array<uint8_t,4>seed{0xa1,0xb2,0xc3,0xd4};
 for(unsigned bits=0;bits<8;++bits){std::array<float,3>normal;for(unsigned a=0;a<3;++a)normal[a]=(bits&(1u<<a))?-1.f:1.f;const auto p=sourceBspCollisionPlaneCandidate(normal,-17,3);require(p.tags()==std::array<uint8_t,4>{3,uint8_t(bits),0,0});}
 require(sourceBspCollisionPlaneCandidate({-0.f,0,-0.f},0,5).signBits==0);
 auto reject=[&](auto action){bool caught=false;try{action();}catch(const std::runtime_error&){caught=true;}require(caught);};reject([&]{sourceBspCollisionPlaneCandidate({1,0,0},0,6);});reject([&]{sourceBspCollisionPlaneCandidate({NAN,0,0},0,0);});reject([&]{sourceBspCollisionPlaneCandidate({1,0,0},INFINITY,0);});
 const std::array<float,3>lo{-1,-1,-1},hi{1,1,1};
 for(unsigned axis=0;axis<3;++axis)for(int sign:{-1,1}){std::array<float,3>start{};start[axis]=float(sign*2);SourceOptimizedBoxLineStateCandidate s;s.planeTags=seed;sourceOptimizedBoxLineRayCandidate(lo,hi,start,{0,0,0},1,s);require(s.planeTags==std::array<uint8_t,4>{uint8_t(axis),sign<0?uint8_t(1u<<axis):uint8_t(0),0xc3,0xd4});}
 for(const auto&ends:std::array<std::array<float,6>,3>{{{-2,3,0,0,3,0},{0,0,0,.5f,0,0},{0,0,0,2,0,0}}}){SourceOptimizedBoxLineStateCandidate s;s.fraction=.25f;s.planeTags=seed;sourceOptimizedBoxLineRayCandidate(lo,hi,{ends[0],ends[1],ends[2]},{ends[3],ends[4],ends[5]},1,s);require(s.planeTags==seed);}
 std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("map read");std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});SourceBspWorldCollisionDataCandidate data(b);for(const auto&p:data.planes)require(p.signBits==sourceBspCollisionPlaneCandidate(p.normal,p.distance,p.type).signBits&&p.reserved==std::array<uint8_t,2>{0,0});
 SourceOptimizedBoxLineStateCandidate state;state.planeTags=seed;state.plane={1,2,3,4};auto before=state;reject([&]{sourceOptimizedBoxLineRayCandidate(lo,hi,{NAN,0,0},{0,0,0},1,state);});require(state.planeTags==before.planeTags&&state.plane==before.plane&&state.fraction==before.fraction);
 std::cout<<"PASS plane tag safety checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
