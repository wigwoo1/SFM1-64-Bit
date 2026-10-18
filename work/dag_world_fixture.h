#pragma once
// Test-only normalized DAG input. This is not a DMX or native ABI definition.
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>
struct DagWorldFixtureNode {
    uint32_t parent=UINT32_MAX, overrideParent=UINT32_MAX, flags=32;
    std::array<float,3> position{};
    std::array<float,4> orientation{0,0,0,1};
    float scale=1;
};
static_assert(sizeof(DagWorldFixtureNode)==44,"fixture wire size");
// flags: disabled=1, pos present=2, pos true=4, rot present=8, rot true=16,
// transform present=32, scale present=64. All graph nodes are supplied as DAGs.
#ifndef DAG_WORLD_FIXTURE_NO_GRAPH
inline Graph dagWorldFixtureGraph(const std::vector<DagWorldFixtureNode>&nodes) {
    Graph g;g.strings={"DmeDag","DmeTransform","node","transform","children",
        "overrideParent","overridePos","overrideRot","disableOverride","position","orientation","scale"};
    g.elements.resize(nodes.size()*2);
    auto reference=[](uint32_t name,uint8_t type,uint32_t id){Value v;v.reference=id;return Attribute{name,type,{v}};};
    auto numeric=[](uint32_t name,uint8_t type,const void*data,size_t bytes){Value v;v.wire.resize(bytes);std::memcpy(v.wire.data(),data,bytes);return Attribute{name,type,{v}};};
    for(uint32_t i=0;i<nodes.size();++i){
        const auto&n=nodes[i];if(n.flags&~127u)throw std::runtime_error("fixture flag bits");
        if((n.parent!=UINT32_MAX&&n.parent>=nodes.size())||(n.overrideParent!=UINT32_MAX&&n.overrideParent>=nodes.size()))throw std::runtime_error("fixture parent span");
        for(float v:n.position)if(!std::isfinite(v))throw std::runtime_error("fixture position");
        for(float v:n.orientation)if(!std::isfinite(v))throw std::runtime_error("fixture orientation");
        if(!std::isfinite(n.scale))throw std::runtime_error("fixture scale");
        auto&e=g.elements[i];e.type=0;e.name=2;
        e.attributes.push_back(reference(3,1,(n.flags&32)?uint32_t(nodes.size())+i:UINT32_MAX));
        e.attributes.push_back(reference(5,1,n.overrideParent));
        const char disabled=(n.flags&1)!=0;e.attributes.push_back(numeric(8,4,&disabled,1));
        if(n.flags&2){const char v=(n.flags&4)!=0;e.attributes.push_back(numeric(6,4,&v,1));}
        if(n.flags&8){const char v=(n.flags&16)!=0;e.attributes.push_back(numeric(7,4,&v,1));}
        auto&t=g.elements[nodes.size()+i];t.type=1;t.name=2;
        t.attributes={numeric(9,10,n.position.data(),12),numeric(10,13,n.orientation.data(),16)};
        if(n.flags&64)t.attributes.push_back(numeric(11,3,&n.scale,4));
    }
    for(uint32_t i=0;i<nodes.size();++i)if(nodes[i].parent!=UINT32_MAX){
        auto&attrs=g.elements[nodes[i].parent].attributes;Attribute*children=nullptr;
        for(auto&a:attrs)if(a.name==4)children=&a;
        if(!children){attrs.push_back(Attribute{4,15,{}});children=&attrs.back();}
        Value ref;ref.reference=i;children->values.push_back(ref);
    }
    return g;
}
#endif
inline void validateDagWorldFixture(const std::vector<DagWorldFixtureNode>&nodes){
    if(nodes.empty()||nodes.size()>64)throw std::runtime_error("fixture node count");
    for(const auto&n:nodes){
        if(n.flags&~127u)throw std::runtime_error("fixture flags");
        for(auto p:{n.parent,n.overrideParent})if(p!=UINT32_MAX&&p>=nodes.size())throw std::runtime_error("fixture parent span");
        for(float v:n.position)if(!std::isfinite(v))throw std::runtime_error("fixture position");
        for(float v:n.orientation)if(!std::isfinite(v))throw std::runtime_error("fixture orientation");
        if(!std::isfinite(n.scale))throw std::runtime_error("fixture scale");
    }
    std::vector<uint8_t>visited(nodes.size());
    auto walk=[&](auto&&self,uint32_t id)->void{
        if(id==UINT32_MAX||visited[id]==2)return;if(visited[id]==1)throw std::runtime_error("fixture transform cycle");visited[id]=1;
        const auto&n=nodes[id];for(uint32_t mask:{6u,24u}){
            const auto p=!(n.flags&1)&&(n.flags&mask)==mask&&n.overrideParent!=UINT32_MAX?n.overrideParent:n.parent;
            self(self,p);
        }visited[id]=2;
    };
    for(uint32_t id=0;id<nodes.size();++id)walk(walk,id);
}
