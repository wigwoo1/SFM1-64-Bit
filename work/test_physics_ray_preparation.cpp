#include "source_physics_ray_preparation_candidate.h"
#include <iostream>
#include <limits>
int main(){try{
    unsigned checks=0;auto require=[&](bool pass){if(!pass)throw std::runtime_error("physics preparation assertion");++checks;};auto near=[](float a,float b){return std::fabs(a-b)<1e-6f;};
    const auto p=sourcePhysicsRayPreparationCandidate({10,20,30},{3,4,0},{1,2,3},{0,0,0});require(p.ray[0]==9&&p.ray[1]==18&&p.ray[2]==27);require(p.ray[3]==12&&p.ray[4]==22&&p.ray[5]==27);require(p.ray[12]==5&&p.ray[13]==5&&p.ray[14]==.2f&&p.ray[15]==0);
    require(near(p.localStart[0],.2286f)&&near(p.localStart[1],-.6858f)&&near(p.localStart[2],.4572f));require(near(p.localDelta[0],.0762f)&&near(p.localDelta[1],0)&&near(p.localDelta[2],.1016f));require(near(p.localDirection[0],.6f)&&near(p.localDirection[2],.8f));require(near(p.lengthMetres,.127f)&&p.minimum==0&&p.maximum==5&&p.radius==0&&p.tolerance==.03125f);
    const auto zero=sourcePhysicsRayPreparationCandidate({0,0,0},{0,0,0},{0,0,0},{0,0,0});require(zero.ray[12]==0&&zero.ray[14]==0&&zero.maximum==9.99999993922529e-09f);require(zero.localDirection==zero.localDelta);
    const auto small=sourcePhysicsRayPreparationCandidate({0,0,0},{.0001f,0,0},{0,0,0},{0,0,0});require(small.localDirection==small.localDelta);const auto larger=sourcePhysicsRayPreparationCandidate({0,0,0},{.001f,0,0},{0,0,0},{0,0,0});require(near(larger.localDirection[0],1));
    auto reject=[&](auto f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}require(failed);};const float inf=std::numeric_limits<float>::infinity(),nan=std::numeric_limits<float>::quiet_NaN(),max=std::numeric_limits<float>::max();
    reject([&]{sourcePhysicsRayPreparationCandidate({nan,0,0},{1,0,0},{0,0,0},{0,0,0});});reject([&]{sourcePhysicsRayPreparationCandidate({0,0,0},{inf,0,0},{0,0,0},{0,0,0});});reject([&]{sourcePhysicsRayPreparationCandidate({0,0,0},{1,0,0},{0,inf,0},{0,0,0});});reject([&]{sourcePhysicsRayPreparationCandidate({0,0,0},{1,0,0},{0,0,0},{0,0,nan});});reject([&]{sourcePhysicsRayPreparationCandidate({0,0,0},{1,0,0},{0,0,0},{36001,0,0});});reject([&]{sourcePhysicsRayPreparationCandidate({max,0,0},{1,0,0},{-max,0,0},{0,0,0});});reject([&]{sourcePhysicsRayPreparationCandidate({0,0,0},{1e30f,0,0},{0,0,0},{0,0,0});});reject([&]{sourcePhysicsRayPreparationCandidate({0,0,0},{std::numeric_limits<float>::denorm_min(),0,0},{0,0,0},{0,0,0});});
    std::cout<<"PASS physics preparation analytical/safety checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
