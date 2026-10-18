#include "source_convex_brush_ray_candidate.h"
#include <iostream>
static void word(std::vector<uint8_t>&b,size_t p,uint32_t value){std::memcpy(b.data()+p,&value,4);}
static std::vector<uint8_t>fixture(){
 std::vector<uint8_t>b(1036);word(b,0,0x50534256);word(b,4,20);
 auto append=[&](size_t id,const std::vector<uint8_t>&payload,uint32_t version){word(b,8+id*16,uint32_t(b.size()));word(b,12+id*16,uint32_t(payload.size()));word(b,16+id*16,version);b.insert(b.end(),payload.begin(),payload.end());};
 std::vector<uint8_t>planes(120);for(size_t i=0;i<6;++i){std::array<float,4>v{};v[i/2]=(i%2)?-1.f:1.f;v[3]=1;std::memcpy(planes.data()+i*20,v.data(),16);word(planes,i*20+16,3);}append(1,planes,0);append(5,{},0);
 std::vector<uint8_t>leaves(32);leaves[26]=1;append(10,leaves,1);std::vector<uint8_t>models(48);word(models,36,0xffffffff);append(14,models,0);append(17,{0,0},0);
 std::vector<uint8_t>brush(12);word(brush,4,6);word(brush,8,1);append(18,brush,0);
 std::vector<uint8_t>sides(48);for(size_t i=0;i<6;++i){sides[i*8]=uint8_t(i);sides[i*8+2]=255;sides[i*8+3]=255;sides[i*8+4]=255;sides[i*8+5]=255;}append(19,sides,0);append(26,{},0);return b;
}
static void require(bool value,const char*message){if(!value)throw std::runtime_error(message);}
int main(){try{
 SourceBspWorldCollisionDataCandidate data(fixture());size_t cases=0;
 auto ray=[&](std::array<float,3>a,std::array<float,3>b){SourceConvexBrushRayStateCandidate s;sourceConvexBrushRayCandidate(data,0,a,b,s);++cases;return s;};
 for(size_t axis=0;axis<3;++axis)for(float sign:{-1.f,1.f}){std::array<float,3>a{},b{};a[axis]=-2*sign;b[axis]=2*sign;const auto s=ray(a,b);require(s.fraction==0.2421875f&&!s.startSolid&&!s.allSolid&&s.contents==1,"axial entry tolerance");}
 auto s=ray({2,2,0},{2,-2,0});require(s.fraction==1&&!s.startSolid,"outside miss");
 s=ray({0,0,0},{0.5f,0,0});require(s.startSolid&&s.allSolid&&s.fraction==0&&s.leftSolid==1,"inside all-solid");
 s=ray({0,0,0},{2,0,0});require(s.startSolid&&!s.allSolid&&s.fraction==1&&s.leftSolid==0.484375f,"inside exit");
 s=ray({1,0,0},{1,0,0});require(s.startSolid&&s.allSolid,"surface stationary native-inside convention");
 s=ray({2,0,0},{2,0,0});require(!s.startSolid&&s.fraction==1,"stationary outside");
 SourceConvexBrushRayStateCandidate prior;prior.fraction=0.125f;sourceConvexBrushRayCandidate(data,0,{-2,0,0},{2,0,0},prior);require(prior.fraction==0.125f,"existing nearer hit replaced");++cases;
 SourceConvexBrushRayStateCandidate merged;merged.fraction=0.5f;merged.leftSolid=0.25f;merged.startSolid=true;merged.side=0;sourceConvexBrushRayCandidate(data,0,{-2,0,0},{2,0,0},merged);require(merged.fraction==1.0f&&merged.leftSolid==0.7421875f&&merged.side==0,"merged solid exit must retain previous plane");++cases;
 auto bevel=data;bevel.sides[1].bevel=1;SourceConvexBrushRayStateCandidate skipped;sourceConvexBrushRayCandidate(bevel,0,{-2,0,0},{0,0,0},skipped);require(skipped.startSolid&&skipped.allSolid,"bevel line side not skipped");++cases;
 auto oblique=data;for(size_t i=0;i<6;++i){auto n=oblique.planes[i].normal;oblique.planes[i].normal={n[0]*0.6f-n[1]*0.8f,n[0]*0.8f+n[1]*0.6f,n[2]};}SourceConvexBrushRayStateCandidate rotated;sourceConvexBrushRayCandidate(oblique,0,{-1.2f,-1.6f,0},{1.2f,1.6f,0},rotated);require(std::fabs(rotated.fraction-0.2421875f)<0.000001f,"general plane entry");++cases;
 size_t rejected=0;for(int i=0;i<3;++i){auto original=prior;try{auto a=std::array<float,3>{0,0,0};auto bad=prior;if(i==0)a[0]=std::numeric_limits<float>::quiet_NaN();if(i==1)bad.fraction=-1;sourceConvexBrushRayCandidate(data,i==2?99u:0u,a,{0,0,0},bad);}catch(const std::exception&){++rejected;}require(prior.fraction==original.fraction,"input state changed");}
 require(rejected==3,"invalid ray/state/index accepted");std::cout<<"PASS "<<cases<<" controlled general brush cases; 3 invalid cases; native oracle is a separate test\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
