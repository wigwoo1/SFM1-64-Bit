#pragma once
#include "graph_transform_hierarchy.h"
#include "transform_world_to_local.h"
#include "transform_scale.h"
#include "session_vector_runtime.h"
// Native 104C6F80: preserve world rotation, replace world translation,
// convert through the parents, then write only local position.
inline bool writeSessionWorldPosition(Graph&g,GraphTransformHierarchy&hierarchy,
    uint32_t dag,const std::array<float,3>&position,SessionAttributeChanges*changes=nullptr) {
    if(&hierarchy.graph!=&g)throw std::runtime_error("hierarchy graph mismatch");
    hierarchy.cache.clear(); // Parent graph and local fields may have changed.
    auto refs=hierarchyRefs(g,g.elements.at(dag),"transform",1);
    if(refs.empty())return false;
    auto world=hierarchy.world(dag);
    for(unsigned row=0;row<3;++row)world[row][3]=position[row];
    const auto pp=hierarchy.parent(dag,true),rp=hierarchy.parent(dag,false);
    const auto pm=hierarchy.world(pp),rm=hierarchy.world(rp);
    auto local=convertRecoveredWorldToLocal(world,pm,recoveredTransformScaleCandidate(pm),
        rm,recoveredTransformScaleCandidate(rm),pp==rp);
    // Validate both transform scalar fields before reaching the writer.
    graphLocalTransformMatrix(g,refs[0]);
    ChannelEndpoint destination{refs[0],"position",0,10};
    std::array<float,3> translated{local[0][3],local[1][3],local[2][3]};
    const bool changed=runtimeWriteVector(g,destination,translated,changes);
    hierarchy.cache.clear();
    return changed;
}
