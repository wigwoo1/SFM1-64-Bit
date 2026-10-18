#include "source_displacement_collision_tree_candidate.h"
#include "source_displacement_grid_triangles_candidate.h"
#include <iostream>
int main(){try{
 std::vector<std::array<float,3>>vertices;for(uint32_t y=0;y<5;++y)for(uint32_t x=0;x<5;++x)vertices.push_back({float(x),float(y),0});
 std::vector<std::array<uint16_t,3>>triangles;for(const auto&t:sourceDisplacementGridTrianglesCandidate(2,std::vector<uint16_t>(32)))triangles.push_back(t.vertices);
 const auto tree=sourceDisplacementCollisionTreeCandidate(2,vertices,triangles);
 const std::array<float,24>root={0,2,0,2,0,0,2,2,0,0,0,0,2,4,2,4,2,2,4,4,0,0,0,0};
 if(tree.nodes.size()!=5||tree.leafPairs.size()!=16||tree.nodes[0].bounds!=root||tree.tightBounds!=std::array<float,6>{0,0,0,4,4,0}||tree.expandedBounds!=std::array<float,6>{-1,-1,-1,5,5,1})throw std::runtime_error("analytical tree bounds");
 const std::array<uint16_t,16>firstIds={0,2,8,10,4,6,12,14,16,18,24,26,20,22,28,30};
 for(size_t i=0;i<firstIds.size();++i)if(tree.leafPairs[i]!=std::array<uint16_t,2>{firstIds[i],uint16_t(firstIds[i]+1)})throw std::runtime_error("analytical Morton mapping");
 unsigned rejected=0;auto rejects=[&](const std::function<void()>&f){try{f();}catch(const std::runtime_error&){++rejected;return;}throw std::runtime_error("invalid input accepted");};
 rejects([&]{sourceDisplacementCollisionTreeCandidate(0,vertices,triangles);});rejects([&]{sourceDisplacementCollisionTreeCandidate(32,vertices,triangles);});
 auto badVertices=vertices;badVertices.pop_back();rejects([&]{sourceDisplacementCollisionTreeCandidate(2,badVertices,triangles);});
 auto badTriangles=triangles;badTriangles.pop_back();rejects([&]{sourceDisplacementCollisionTreeCandidate(2,vertices,badTriangles);});
 badTriangles=triangles;badTriangles[0][0]=511;rejects([&]{sourceDisplacementCollisionTreeCandidate(2,vertices,badTriangles);});
 badVertices=vertices;badVertices[0][0]=std::numeric_limits<float>::quiet_NaN();rejects([&]{sourceDisplacementCollisionTreeCandidate(2,badVertices,triangles);});
 if(rejected!=6)throw std::runtime_error("guard coverage");
 std::cout<<"PASS analytical mapping, node/root/expanded bounds and six guards\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
