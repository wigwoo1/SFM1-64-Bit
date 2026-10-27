#pragma once
#include "clip_hierarchy.h"
#include "model_proxy_context_value_candidate.h"

inline SessionModelProxyContextCandidate readSessionModelProxyContextCandidate(const Graph&g,uint32_t model){
    SessionModelProxyContextCandidate out;
    if(model>=g.elements.size())return out;
    const auto&e=g.elements[model];
    if(e.type>=g.strings.size()||g.strings[e.type]!="DmeGameModel")return out;
    const Attribute*name=nullptr;
    for(const auto&a:e.attributes){
        if(a.name>=g.strings.size())return out;
        if(g.strings[a.name]=="modelName"){if(name)return out;name=&a;}
    }
    if(!name||name->type!=5||name->values.size()!=1)return out;
    const auto&asset=name->values.front().text;
    // Original CMovieDoc::CreatePuppets102ABCC0: the ordinary model branch
    // admits nonempty modelName other than "?", with '*' names sent through
    // an additional model-index service. Missing/exception paths may use
    // effects. Those services are not inferred from the session string.
    if(asset.empty()||asset=="?"||asset.front()=='*'||asset.find('\0')!=std::string::npos)return out;
    out.model_=model;out.guid_=e.id;out.asset_=asset;out.ordinaryPuppet_=true;
    return out;
}
