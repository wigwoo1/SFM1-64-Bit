#pragma once
#include "graph_local_transform.h"
#include "transform_local_to_world.h"
#include "transform_orientation.h"
#include <map>
#include <set>
struct GraphTransformHierarchy {
    const Graph& graph;
    std::map<uint32_t,uint32_t> ordinaryParents;
    std::map<uint32_t,RecoveredMatrix3x4> cache;
    std::set<uint32_t> visiting;
    explicit GraphTransformHierarchy(const Graph&g,uint32_t scopeRoot=0xffffffff):graph(g) {
        std::set<uint32_t> scope;
        if(scopeRoot==0xffffffff){for(uint32_t id=0;id<g.elements.size();++id)scope.insert(id);}
        else {
            std::set<uint32_t> visiting;
            auto collect=[&](auto&&self,uint32_t id)->void{
                if(id>=g.elements.size())throw std::runtime_error("transform scope reference");
                if(scope.count(id))return;
                if(!visiting.insert(id).second)throw std::runtime_error("cyclic transform scope");
                scope.insert(id);
                for(auto child:hierarchyRefs(g,g.elements[id],"children",15))self(self,child);
                visiting.erase(id);
            };
            collect(collect,scopeRoot);
        }
        for(uint32_t id:scope) {
            const auto& e=g.elements[id];
            if(!namedAttribute(g,e,"transform"))continue;
            for(auto child:hierarchyRefs(g,e,"children",15)) {
                auto inserted=ordinaryParents.emplace(child,id);
                if(!inserted.second && inserted.first->second!=id)
                    throw std::runtime_error("ambiguous transform parent");
            }
        }
    }
    static RecoveredMatrix3x4 identity() {
        RecoveredMatrix3x4 m{};for(unsigned i=0;i<3;++i)m[i][i]=1.0f;return m;
    }
    std::array<float,4> worldOrientation(uint32_t id) {
        return extractRecoveredTransformOrientationCandidate(world(id));
    }
    bool overridesDisabled(uint32_t id) const {
        // Native CDmeDag bool wrapper at +64, default false (104C29D0).
        const auto* flag=namedAttribute(graph,graph.elements.at(id),"disableOverride");
        if(!flag)return false;
        if(flag->type!=4 || flag->values.size()!=1 || flag->values[0].wire.size()!=1)
            throw std::runtime_error("invalid disableOverride flag");
        return flag->values[0].wire[0]!=0;
    }
    uint32_t parent(uint32_t id,bool position) const {
        const auto& e=graph.elements.at(id);
        const auto* flag=namedAttribute(graph,e,position?"overridePos":"overrideRot");
        if(!overridesDisabled(id) && flag) {
            if(flag->type!=4 || flag->values.size()!=1 || flag->values[0].wire.size()!=1)
                throw std::runtime_error("invalid transform override flag");
            if(flag->values[0].wire[0]) {
                auto refs=hierarchyRefs(graph,e,"overrideParent",1);
                if(!refs.empty())return refs[0];
            }
        }
        const auto it=ordinaryParents.find(id);
        return it==ordinaryParents.end()?0xffffffff:it->second;
    }
    RecoveredMatrix3x4 world(uint32_t id) {
        if(id==0xffffffff)return identity();
        const auto found=cache.find(id);if(found!=cache.end())return found->second;
        if(!visiting.insert(id).second)throw std::runtime_error("cyclic transform hierarchy");
        try {
            const auto& e=graph.elements.at(id);
            auto refs=hierarchyRefs(graph,e,"transform",1);
            auto local=refs.empty()?identity():graphLocalTransformMatrix(graph,refs[0],true);
            const auto pp=parent(id,true),rp=parent(id,false);
            auto result=convertRecoveredLocalToWorld(local,world(pp),world(rp),pp==rp);
            visiting.erase(id);cache.emplace(id,result);return result;
        }catch(...){visiting.erase(id);throw;}
    }
};
