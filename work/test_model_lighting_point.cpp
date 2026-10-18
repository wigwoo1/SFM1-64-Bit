#include "source_model_lighting_point_candidate.h"
#include <iostream>
#include <limits>
int main(){try{
 using Point=std::array<float,3>;const Point local{1,2,3};const RecoveredMatrix3x4 model{{{1,0,0,10},{0,1,0,20},{0,0,1,30}}},attachment{{{2,0,0,100},{0,2,0,200},{0,0,2,300}}};unsigned calls=0;
 auto callback=[&](int32_t index,RecoveredMatrix3x4&out){if(index!=2)throw std::runtime_error("wrong attachment index");++calls;out=attachment;return true;};
 if(sourceModelIlluminationPointCandidate(local,model,2,callback)!=Point{102,204,306}||calls!=1)throw std::runtime_error("attachment selection");
 if(sourceModelIlluminationPointCandidate(local,model,0,callback)!=Point{11,22,33}||calls!=1)throw std::runtime_error("zero attachment queried");if(sourceModelIlluminationPointCandidate(local,model,-1,callback)!=Point{11,22,33}||calls!=1)throw std::runtime_error("negative attachment queried");
 const float bad=std::numeric_limits<float>::infinity();RecoveredMatrix3x4 invalid=model;invalid[0][0]=bad;
 if(sourceModelIlluminationPointCandidate(local,invalid,2,callback)!=Point{102,204,306})throw std::runtime_error("unused model matrix validated");
 if(sourceModelIlluminationPointCandidate(local,model,2,[&](int32_t,auto&out){out=invalid;return false;})!=Point{11,22,33})throw std::runtime_error("failed attachment matrix used");
 const Point explicitPoint{7,8,9};if(sourceModelLightingOriginCandidate({bad,bad,bad},&invalid,&explicitPoint)!=explicitPoint)throw std::runtime_error("explicit origin precedence");if(sourceModelLightingOriginCandidate(local,&model,nullptr)!=Point{11,22,33})throw std::runtime_error("offset transform");if(sourceModelLightingOriginCandidate(local,nullptr,nullptr)!=local)throw std::runtime_error("default point copy");
 unsigned guards=0;auto rejects=[&](auto action){bool failed=false;try{action();}catch(const std::runtime_error&){failed=true;}if(!failed)throw std::runtime_error("invalid selected input accepted");++guards;};
 rejects([&]{sourceModelLightingOriginCandidate({bad,0,0},nullptr,nullptr);});rejects([&]{sourceModelLightingOriginCandidate(local,&invalid,nullptr);});const Point badExplicit{bad,0,0};rejects([&]{sourceModelLightingOriginCandidate(local,&model,&badExplicit);});rejects([&]{sourceModelIlluminationPointCandidate(local,model,2,[&](int32_t,auto&out){out=invalid;return true;});});
 auto huge=model;huge[0][0]=std::numeric_limits<float>::max();rejects([&]{sourceLightingTransformPointCandidate({std::numeric_limits<float>::max(),0,0},huge);});
 if(guards!=5||local!=Point{1,2,3}||model[0][3]!=10)throw std::runtime_error("guard/input preservation");std::cout<<"PASS attachment selection/fallback, explicit origin priority, offset/direct paths, unused-input handling and five finite/overflow guards\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
