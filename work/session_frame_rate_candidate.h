#pragma once
#include "clip_hierarchy.h"
#include "recovered_float_rate_candidate.h"
inline uint32_t sessionFrameRateCandidate(const Graph&g,uint32_t root,bool trace=true){
 const Element*owner=nullptr;
 for(auto&e:g.elements){auto active=hierarchyRefs(g,e,"activeClip",1);if(active.size()==1&&active[0]==root){if(owner)throw std::runtime_error("ambiguous session rate owner");owner=&e;}}
 if(!owner)throw std::runtime_error("session rate owner missing");
 auto settings=hierarchyRefs(g,*owner,"settings",1);if(settings.size()!=1)throw std::runtime_error("session settings missing");
 auto render=hierarchyRefs(g,g.elements.at(settings[0]),"renderSettings",1);if(render.size()!=1)throw std::runtime_error("session render settings missing");
 auto*a=namedAttribute(g,g.elements.at(render[0]),"frameRate");if(!a||a->type!=3||a->values.size()!=1)throw std::runtime_error("session frame rate invalid");
 auto fps=wireFloat(a->values[0]);if(!(fps>0))throw std::runtime_error("session frame rate must be positive");
 auto rate=recoveredFloatRateCandidate(fps);if(!(rate&65535u))throw std::runtime_error("session frame rate converts to zero");if(trace)std::cout<<"session_rate_source=renderSettings fps="<<fps<<" packed="<<rate<<" owner="<<render[0]<<" native_settings_precedence=unverified\n";return rate;
}
