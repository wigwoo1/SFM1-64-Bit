#pragma once
#include "ifm_bone_precompute_candidate.h"
#include <map>
// Requires Graph/namedAttribute and a scoped hierarchy. This retained-wire
// adapter models one loaded file, not native mutable cross-file references.
// Native reverse order is absent in DMX: refuse multiple matching DAG owners.
template<class IsDag,class IsTransform,class World>
std::vector<IfmBoneTransformReferencesCandidate>sessionIfmBoneReferencesCandidate(
    const Graph&g,uint32_t model,uint32_t count,IsDag isDag,IsTransform isTransform,World world){
    if(count>256||g.strings.at(g.elements.at(model).type)!="DmeGameModel")throw std::runtime_error("bone references model/count");
    const auto*a=namedAttribute(g,g.elements.at(model),"bones");
    if(!a||a->type!=15||a->values.size()!=count)throw std::runtime_error("bone references require matching array count");
    std::map<uint32_t,std::vector<uint32_t>>owners;
    for(uint32_t id=0;id<g.elements.size();++id){if(!isDag(id))continue;const auto*t=namedAttribute(g,g.elements[id],"transform");if(!t)continue;
        if(t->type!=1||t->values.size()!=1)throw std::runtime_error("bone reference transform schema");
        const auto ref=t->values[0].reference;if(ref==UINT32_MAX)continue;if(ref>=g.elements.size())throw std::runtime_error("bone reference external transform");owners[ref].push_back(id);
    }
    std::vector<IfmBoneTransformReferencesCandidate>out(count);
    for(uint32_t i=0;i<count;++i){auto&b=out[i];const auto transform=a->values[i].reference;b.present=transform!=UINT32_MAX;
        if(!b.present){b.transformTyped=false;continue;}if(transform>=g.elements.size())throw std::runtime_error("bone reference external bone");b.transformTyped=isTransform(transform);if(!b.transformTyped)continue;
        const auto found=owners.find(transform);if(found==owners.end())continue;
        if(found->second.size()>1)throw std::runtime_error("bone reference order ambiguous; native context required");
        const auto id=found->second[0];const auto&e=g.elements[id];IfmBoneDagReferenceCandidate r;
        const auto*disabled=namedAttribute(g,e,"disableOverride");if(disabled){if(disabled->type!=4||disabled->values.size()!=1||disabled->values[0].wire.size()!=1)throw std::runtime_error("bone override disable schema");r.overridesDisabled=disabled->values[0].wire[0]!=0;}
        const auto*parent=namedAttribute(g,e,"overrideParent");r.overridePresent=false;r.overrideDagTyped=false;
        if(parent){if(parent->type!=1||parent->values.size()!=1)throw std::runtime_error("bone override parent schema");const auto target=parent->values[0].reference;
            if(target!=UINT32_MAX){if(target>=g.elements.size())throw std::runtime_error("bone override external parent");r.overridePresent=true;r.overrideDagTyped=isDag(target);}}
        if(!r.overridesDisabled&&r.overridePresent&&r.overrideDagTyped)r.world=world(id);
        b.references.push_back(r);
    }return out;
}
