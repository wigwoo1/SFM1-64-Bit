#pragma once
#include "source_phy_compact_geometry_candidate.h"
struct SourcePhysicsTreeStateCandidate {
    std::array<float,3>midpoint{},start{},direction{};
    float length=0,boxRadius=0;
};
inline void sourcePhysicsValidateTreeCandidate(const std::vector<SourcePhyCollisionTreeNodeCandidate>&tree){
    if(tree.empty()||tree.size()>1000000)throw std::runtime_error("invalid collision tree size");std::vector<uint32_t>pending{0};std::vector<bool>seen(tree.size(),false);size_t count=0;
    while(!pending.empty()){const uint32_t index=pending.back();pending.pop_back();if(index>=tree.size()||seen[index])throw std::runtime_error("collision tree repeated/outside node");seen[index]=true;++count;const auto&n=tree[index];for(unsigned i=2;i<6;++i){float x;std::memcpy(&x,&n.raw[i],4);if(!std::isfinite(x))throw std::runtime_error("nonfinite collision node bounds");}if(n.raw[0]){if(n.left==UINT32_MAX||n.right==UINT32_MAX||n.ledge!=UINT32_MAX)throw std::runtime_error("invalid collision internal links");pending.push_back(n.right);pending.push_back(n.left);}else if(n.left!=UINT32_MAX||n.right!=UINT32_MAX||n.ledge==UINT32_MAX)throw std::runtime_error("invalid collision leaf links");}
    if(count!=tree.size())throw std::runtime_error("unreachable collision tree node");
}
// Original2AD00 root rejection, child pruning, near-first ordering/right ties
// and pending-node semantics. The leaf callback owns collision/hit updates.
// A pending node is NOT retested against its own bounds after a previous hit.
template<class Leaf>
void sourcePhysicsTreeTraversalCandidate(const std::vector<SourcePhyCollisionTreeNodeCandidate>&tree,
    SourcePhysicsTreeStateCandidate&state,Leaf&&leaf){
    sourcePhysicsValidateTreeCandidate(tree);auto currentState=state;
    auto finite=[](float x){if(!std::isfinite(x))throw std::runtime_error("nonfinite collision tree arithmetic");return x;};
    auto validateState=[&](){for(const auto&v:{currentState.midpoint,currentState.start,currentState.direction})for(float x:v)finite(x);finite(currentState.length);finite(currentState.boxRadius);};validateState();
    auto center=[&](uint32_t index){std::array<float,3>v;std::memcpy(v.data(),tree[index].raw.data()+2,12);return v;};
    auto eligible=[&](uint32_t index){const auto c=center(index);std::array<float,3>d;for(unsigned j=0;j<3;++j)d[j]=finite(c[j]-currentState.midpoint[j]);float radius;std::memcpy(&radius,&tree[index].raw[5],4);radius=finite(radius+currentState.boxRadius);const float squared=finite(radius*radius);std::array<float,3>v=d;if(currentState.length>0){const auto&u=currentState.direction;v={finite(u[1]*d[2]-u[2]*d[1]),finite(u[2]*d[0]-u[0]*d[2]),finite(u[0]*d[1]-u[1]*d[0])};}const float distance=finite((v[0]*v[0]+v[1]*v[1])+v[2]*v[2]);return squared>distance;};
    auto distance=[&](uint32_t index){const auto c=center(index);std::array<float,3>d;for(unsigned j=0;j<3;++j)d[j]=finite(currentState.start[j]-c[j]);return finite((d[0]*d[0]+d[1]*d[1])+d[2]*d[2]);};
    if(!eligible(0))return;std::vector<uint32_t>pending;uint32_t current=0;
    for(;;){
        const auto&node=tree[current];
        if(!node.raw[0]){leaf(current,node.ledge,currentState);validateState();}
        else{const bool left=eligible(node.left),right=eligible(node.right);if(left&&right){if(distance(node.right)<=distance(node.left)){pending.push_back(node.left);current=node.right;}else{pending.push_back(node.right);current=node.left;}continue;}if(left||right){current=left?node.left:node.right;continue;}}
        if(pending.empty())break;current=pending.back();pending.pop_back();
    }
    state=currentState;
}
