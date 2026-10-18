#include "source_displacement_runtime_mesh_candidate.h"
#include <iostream>
int main(){try{
 const std::array<std::array<float,3>,4>corners={{{0,0,0},{0,16,0},{16,16,0},{16,0,0}}};std::vector<std::array<float,3>>positions;for(uint32_t y=0;y<3;++y)for(uint32_t x=0;x<3;++x)positions.push_back({float(x*8),float(y*8),0});
 auto triangles=sourceDisplacementGridTrianglesCandidate(1,std::vector<uint16_t>(8));std::vector<float>alphas(9,127.5f);
 const auto tie=sourceDisplacementRuntimeMeshCandidate(1,1,8,corners,positions,alphas,triangles);
 if(tie.power!=1||tie.contents!=1||tie.runtimeFlags!=8||tie.corners!=corners||tie.vertices!=positions||tie.baseNormal!=std::array<float,3>{0,0,1}||tie.tree.expandedBounds!=std::array<float,6>{-1,-1,-1,17,17,1})throw std::runtime_error("analytical runtime fields/bounds");
 for(const auto&t:tie.triangles)if((t.flags&0x7c0)!=0x240||t.normal!=std::array<float,3>{0,0,1}||t.distance!=0)throw std::runtime_error("alpha threshold tie/plane");
 auto highAlphas=alphas;for(float&x:highAlphas)x=std::nextafter(x,128.f);const auto high=sourceDisplacementRuntimeMeshCandidate(1,1,0,corners,positions,highAlphas,triangles);if((high.triangles[0].flags&0x7c0)!=0x440)throw std::runtime_error("strict high alpha branch");
 triangles[0].tags=16;triangles[0].vertices[0]|=0xfe00;const auto both=sourceDisplacementRuntimeMeshCandidate(1,1,0,corners,positions,alphas,triangles);if((both.triangles[0].flags&0x7c0)!=0x640||(both.triangles[0].packedVertices[0]&511u)!=0)throw std::runtime_error("tag alpha OR semantics or nine-bit masking");
 unsigned rejected=0;auto rejects=[&](const std::function<void()>&f){try{f();}catch(const std::runtime_error&){++rejected;return;}throw std::runtime_error("invalid input accepted");};
 auto call=[&](uint32_t power,const auto&c,const auto&p,const auto&a,const auto&t){sourceDisplacementRuntimeMeshCandidate(power,1,0,c,p,a,t);};
 rejects([&]{call(0,corners,positions,alphas,triangles);});rejects([&]{call(32,corners,positions,alphas,triangles);});
 auto badPositions=positions;badPositions.pop_back();rejects([&]{call(1,corners,badPositions,alphas,triangles);});
 auto badAlphas=alphas;badAlphas.pop_back();rejects([&]{call(1,corners,positions,badAlphas,triangles);});
 auto badTriangles=triangles;badTriangles.pop_back();rejects([&]{call(1,corners,positions,alphas,badTriangles);});
 badTriangles=triangles;badTriangles[0].vertices[0]=511;rejects([&]{call(1,corners,positions,alphas,badTriangles);});
 badAlphas=alphas;badAlphas[0]=std::numeric_limits<float>::quiet_NaN();rejects([&]{call(1,corners,positions,badAlphas,triangles);});
 badPositions=positions;badPositions[0][0]=std::numeric_limits<float>::infinity();rejects([&]{call(1,corners,badPositions,alphas,triangles);});
 auto badCorners=corners;badCorners[0][0]=std::numeric_limits<float>::quiet_NaN();rejects([&]{call(1,badCorners,positions,alphas,triangles);});
 badAlphas.assign(9,std::numeric_limits<float>::max());rejects([&]{call(1,corners,positions,badAlphas,triangles);});
 if(rejected!=10||positions.size()!=9||alphas[0]!=127.5f)throw std::runtime_error("guard coverage or input mutation");std::cout<<"PASS runtime fields/bounds/planes, alpha tie/high/tag OR/masking, ten guards\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
