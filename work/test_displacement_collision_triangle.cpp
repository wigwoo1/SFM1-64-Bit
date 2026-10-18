#include "source_displacement_collision_triangle_candidate.h"
#include <functional>
#include <iostream>
#include <limits>
int main(){try{
 const std::vector<std::array<float,3>>vertices={{{0,0,2}},{{0,8,2}},{{8,0,2}}};
 SourceDisplacementCollisionTriangleCandidate initial;initial.packedVertices={0xe000,0xe001,0xe002};initial.flags=0xffff;
 const auto result=sourceDisplacementCollisionTriangleCandidate(vertices,initial);
 if(result.normal!=std::array<float,3>{0,0,1}||result.distance!=2||result.flags!=0xffd0||result.packedVertices!=std::array<uint16_t,3>{0xf000,0xe801,0xe002})throw std::runtime_error("analytical plane/flags/extrema");
 auto coincident=vertices;coincident[1]=coincident[2]=coincident[0];const auto zero=sourceDisplacementCollisionTriangleCandidate(coincident,{});
 if(zero.normal!=std::array<float,3>{0,0,0}||zero.distance!=0||(zero.flags&0x3f)!=0x28)throw std::runtime_error("degenerate plane");
 unsigned rejected=0;auto rejects=[&](const std::function<void()>&f){try{f();}catch(const std::runtime_error&){++rejected;return;}throw std::runtime_error("invalid input accepted");};
 auto bad=initial;bad.packedVertices[0]=3;rejects([&]{sourceDisplacementCollisionTriangleCandidate(vertices,bad);});
 rejects([&]{sourceDisplacementCollisionTriangleCandidate({},initial);});
 auto nonfinite=vertices;nonfinite[0][0]=std::numeric_limits<float>::quiet_NaN();rejects([&]{sourceDisplacementCollisionTriangleCandidate(nonfinite,initial);});
 nonfinite=vertices;nonfinite[0][0]=std::numeric_limits<float>::infinity();rejects([&]{sourceDisplacementCollisionTriangleCandidate(nonfinite,initial);});
 nonfinite={{{0,0,0}},{{1.e30f,0,0}},{{0,1.e30f,0}}};rejects([&]{sourceDisplacementCollisionTriangleCandidate(nonfinite,initial);});
 if(rejected!=5||initial.flags!=0xffff||initial.packedVertices[0]!=0xe000)throw std::runtime_error("guard coverage or input mutation");
 std::cout<<"PASS analytical plane/flags/extrema, degenerate triangle, five guards\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
