#include "source_physics_triangle_contact_time_candidate.h"
#include <cstring>
#include <iostream>
#include <limits>
int main(){try{
    unsigned checks=0;auto require=[&](bool pass){if(!pass)throw std::runtime_error("contact time assertion");++checks;};SourcePhysicsSimplexCandidate s;s.vertices[0].point={0,0,0};s.vertices[1].point={1,0,0};s.vertices[2].point={0,1,0};const auto before=s;
    const std::array<float,3>zero{};const float sentinel=1.0000000138484279e24f;require(sourcePhysicsTriangleContactTimeCandidate(s,zero,0)==sentinel);require(sourcePhysicsTriangleContactTimeCandidate(s,{0,0,1},1)==sentinel);
    require(sourcePhysicsTriangleContactTimeCandidate(s,{1,0,0},0)==1);require(sourcePhysicsTriangleContactTimeCandidate(s,{1,1,0},0)==.5f);require(sourcePhysicsTriangleContactTimeCandidate(s,{-1,-1,0},0)==0);require(sourcePhysicsTriangleContactTimeCandidate(s,{0,-1,0},0)==0);
    const auto n=sourcePhysicsContactNormalizeCandidate({3,4,0});require(n==std::array<float,3>{.6f,.8f,0});require(sourcePhysicsContactNormalizeCandidate({0,0,0})==zero);const auto signedZero=sourcePhysicsContactNormalizeCandidate({-0.f,0.f,-0.f});require(std::signbit(signedZero[0])&&!std::signbit(signedZero[1])&&std::signbit(signedZero[2]));
    auto invalid=[&](SourcePhysicsSimplexCandidate shape,std::array<float,3>d,float tolerance){const auto old=shape;bool rejected=false;try{sourcePhysicsTriangleContactTimeCandidate(shape,d,tolerance);}catch(const std::runtime_error&){rejected=true;}require(rejected);require(std::memcmp(&shape,&old,sizeof(shape))==0);};
    auto bad=s;bad.vertices[0].point[0]=std::numeric_limits<float>::quiet_NaN();invalid(bad,{1,2,3},.1f);invalid(s,{0,std::numeric_limits<float>::infinity(),0},0);invalid(s,{1,0,0},std::numeric_limits<float>::quiet_NaN());bad=s;bad.vertices[1].point[0]=3e38f;bad.vertices[2].point[1]=3e38f;invalid(bad,{1,2,3},0);
    s.count=0xffffffff;s.vertices[3].point.fill(std::numeric_limits<float>::quiet_NaN());require(sourcePhysicsTriangleContactTimeCandidate(s,zero,1)==sentinel);require(before.vertices[0].point==s.vertices[0].point);std::cout<<"PASS contact-time analytical/guard checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
