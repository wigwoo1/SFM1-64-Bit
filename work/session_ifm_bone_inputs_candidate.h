#pragma once
#include "ifm_bone_world_palette_candidate.h"
// Requires Graph/namedAttribute. Original10241A50 reads typed bone transforms
// by ARRAY INDEX, not by label/name. Missing/untyped entries become zero
// translation + identity quaternion + scale1. This only prepares local inputs;
// procedural/override-bone/cache/model root-frame selection remain separate.
template<class IsTransform>
std::vector<IfmBoneLocalPoseCandidate>sessionIfmBoneInputsCandidate(
    const Graph&g,uint32_t model,const std::vector<uint8_t>&mdl,IsTransform isTransform){
    if(g.strings.at(g.elements.at(model).type)!="DmeGameModel")throw std::runtime_error("IFM pose needs game model");
    auto integer=[&](size_t at){if(at>mdl.size()||mdl.size()-at<4)throw std::runtime_error("IFM MDL span");int32_t v;std::memcpy(&v,mdl.data()+at,4);return v;};
    if(integer(0)!=0x54534449||integer(4)!=49||integer(76)<0||size_t(integer(76))!=mdl.size())throw std::runtime_error("IFM MDL49 identity/size");
    const auto count=integer(156),offset=integer(160);
    if(count<0||count>256||offset<0||uint64_t(offset)+uint64_t(count)*216>mdl.size())throw std::runtime_error("IFM MDL bone table");
    const auto*a=namedAttribute(g,g.elements.at(model),"bones");if(!a||a->type!=15||a->values.size()>256)throw std::runtime_error("IFM session bone array");
    std::vector<IfmBoneLocalPoseCandidate>out(static_cast<size_t>(count));
    for(size_t i=0;i<out.size();++i){auto&b=out[i];b.parent=integer(size_t(offset)+i*216+4);std::memcpy(&b.flags,mdl.data()+size_t(offset)+i*216+160,4);
        if(b.parent< -1||(b.parent>=0&&size_t(b.parent)>=i))throw std::runtime_error("IFM MDL parent order");
        if(i>=a->values.size())continue;const auto id=a->values[i].reference;if(id==UINT32_MAX)continue;
        if(id>=g.elements.size())throw std::runtime_error("IFM bone external/unresolved reference");if(!isTransform(id))continue;
        const auto&e=g.elements[id];const auto*p=namedAttribute(g,e,"position"),*q=namedAttribute(g,e,"orientation"),*s=namedAttribute(g,e,"scale");
        if(!p||p->type!=10||p->values.size()!=1||p->values[0].wire.size()!=12||!q||q->type!=13||q->values.size()!=1||q->values[0].wire.size()!=16)throw std::runtime_error("IFM bone transform schema");
        std::memcpy(b.position.data(),p->values[0].wire.data(),12);std::memcpy(b.orientation.data(),q->values[0].wire.data(),16);
        if(s){if(s->type!=3||s->values.size()!=1||s->values[0].wire.size()!=4)throw std::runtime_error("IFM bone scale conversion unsupported");std::memcpy(&b.scale,s->values[0].wire.data(),4);}
        for(float v:b.position)if(!std::isfinite(v))throw std::runtime_error("IFM bone position finite");for(float v:b.orientation)if(!std::isfinite(v))throw std::runtime_error("IFM bone orientation finite");if(!std::isfinite(b.scale))throw std::runtime_error("IFM bone scale finite");
    }return out;
}
