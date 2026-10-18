#pragma once
#include "animation_keys.h"
#include "clip_hierarchy.h"
#include "vector_curve_sampler.h"
// Requires Graph reader declarations. Bind explicit log owner, never infer one.
inline RecoveredVectorCurveSampler bindGraphVectorCurveSampler(const Graph&g,uint32_t logIndex,uint32_t layerIndex,const std::array<float,3>&disabledEdgeValue){
 auto field=[&](const Element&e,const char*name,uint8_t type)->const Attribute&{for(auto&a:e.attributes)if(g.strings.at(a.name)==name){if(a.type!=type)throw std::runtime_error("curve field type mismatch");return a;}throw std::runtime_error("missing curve field");};
 const auto&log=g.elements.at(logIndex);const auto&layer=g.elements.at(layerIndex);
 if(g.strings.at(log.type)!="DmeVector3Log"||g.strings.at(layer.type)!="DmeVector3LogLayer")throw std::runtime_error("wrong float curve classes");
 auto owned=hierarchyRefs(g,log,"layers",15);if(std::find(owned.begin(),owned.end(),layerIndex)==owned.end())throw std::runtime_error("vector layer owner mismatch");
 const auto&ref=field(log,"curveinfo",1);if(ref.values.size()!=1||ref.values[0].reference>=g.elements.size())throw std::runtime_error("curveinfo requires live local target");const auto&info=g.elements.at(ref.values[0].reference);
 if(g.strings.at(info.type)!="DmeVector3CurveInfo")throw std::runtime_error("curveinfo class evaluator not bound");
 auto scalar=[&](const char*name,uint8_t type)->const Value&{auto&a=field(info,name,type);if(a.values.size()!=1)throw std::runtime_error("invalid curve scalar count");return a.values[0];};
 auto vectorValue=[&](const Value&v){if(v.wire.size()!=12)throw std::runtime_error("invalid vector field width");std::array<float,3>r;std::memcpy(r.data(),v.wire.data(),12);return r;};
 auto boolean=[&](const char*name){auto&v=scalar(name,4);if(v.wire.size()!=1)throw std::runtime_error("invalid curve boolean");return v.wire[0]!=0;};
 RecoveredVectorCurveSampler s;auto&c=s.info;c.defaultKind=wireInt(scalar("defaultCurveType",2));c.rightEdgeTime=wireInt(scalar("rightEdgeTime",7));c.useEdgeInfo=boolean("useEdgeInfo");c.leftActive=boolean("leftEdgeActive");c.rightActive=boolean("rightEdgeActive");c.leftValue=vectorValue(scalar("leftEdgeValue",10));c.rightValue=vectorValue(scalar("rightEdgeValue",10));c.defaultEdgeZero=vectorValue(scalar("defaultEdgeZeroValue",10));c.leftKind=wireInt(scalar("leftEdgeCurveType",2));c.rightKind=wireInt(scalar("rightEdgeCurveType",2));c.disabledEdgeValue=disabledEdgeValue;
 KeyTrack track(g,layer);if(track.values->type!=24)throw std::runtime_error("float curve value array required");for(auto&v:track.times->values)s.times.push_back(wireInt(v));for(auto&v:track.values->values)s.values.push_back(vectorValue(v));for(auto&v:field(layer,"curvetypes",16).values)s.kinds.push_back(wireInt(v));return s;
}
