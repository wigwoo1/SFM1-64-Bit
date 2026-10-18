#pragma once
#include "source_physics_tree_traversal_candidate.h"
#include "physics_support_test_fixture.h"
inline void treeWrite(std::ostream&out,const std::vector<SourcePhyCollisionTreeNodeCandidate>&tree){supportPut(out,uint32_t(tree.size()));for(const auto&n:tree){supportPut(out,n.raw);supportPut(out,n.left);supportPut(out,n.right);supportPut(out,n.ledge);}}
inline std::vector<SourcePhyCollisionTreeNodeCandidate> treeRead(std::istream&in){uint32_t count;supportGet(in,count);if(!count||count>1000000)throw std::runtime_error("tree wire size");std::vector<SourcePhyCollisionTreeNodeCandidate>tree(count);for(auto&n:tree){supportGet(in,n.raw);supportGet(in,n.left);supportGet(in,n.right);supportGet(in,n.ledge);}sourcePhysicsValidateTreeCandidate(tree);return tree;}
inline void treeWriteState(std::ostream&out,const SourcePhysicsTreeStateCandidate&s){supportPut(out,s.midpoint);supportPut(out,s.start);supportPut(out,s.direction);supportPut(out,s.length);supportPut(out,s.boxRadius);}
inline SourcePhysicsTreeStateCandidate treeReadState(std::istream&in){SourcePhysicsTreeStateCandidate s;supportGet(in,s.midpoint);supportGet(in,s.start);supportGet(in,s.direction);supportGet(in,s.length);supportGet(in,s.boxRadius);return s;}
inline bool treeSameState(const SourcePhysicsTreeStateCandidate&a,const SourcePhysicsTreeStateCandidate&b){std::ostringstream x(std::ios::binary),y(std::ios::binary);treeWriteState(x,a);treeWriteState(y,b);return x.str()==y.str();}
