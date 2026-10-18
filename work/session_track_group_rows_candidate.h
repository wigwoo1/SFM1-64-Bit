#pragma once
#include <stdexcept>
#include <string>
#include <vector>
#include "session_render_root_candidate.h"

// SFM shows the special sub-clip track group before the film clip's regular
// trackGroups array. The order is inferred from the dialog's count+1 row
// population and corroborated against the mtt_soldier session graph.
struct SessionTrackGroupRowCandidate { uint32_t element=UINT32_MAX; std::string name; bool visible=true; bool special=false; };
inline std::vector<SessionTrackGroupRowCandidate> sessionTrackGroupRowsCandidate(const Graph& graph) {
 const auto root=sessionRenderRootCandidate(graph);
 const auto& clip=graph.elements.at(root);
 if(graph.strings.at(clip.type)!="DmeFilmClip") throw std::runtime_error("session track groups require a DmeFilmClip root");
 std::vector<SessionTrackGroupRowCandidate> rows;
 auto add=[&](uint32_t id,bool special){
  if(id>=graph.elements.size()) throw std::runtime_error("track group reference outside DMX document");
  const auto& element=graph.elements[id];
  if(graph.strings.at(element.type)!="DmeTrackGroup") throw std::runtime_error("film clip track group reference is not DmeTrackGroup");
  const Attribute* visible=nullptr;
  for(const auto& attribute:element.attributes) if(graph.strings.at(attribute.name)=="visible"){visible=&attribute;break;}
  if(!visible||visible->type!=4||visible->values.size()!=1||visible->values[0].wire.size()!=1)
   throw std::runtime_error("track group visibility is not a scalar boolean");
  rows.push_back({id,graph.strings.at(element.name),visible->values[0].wire[0]!=0,special});
 };
 auto special=hierarchyRefs(graph,clip,"subClipTrackGroup",1);
 if(special.size()>1) throw std::runtime_error("subClipTrackGroup is not scalar");
 if(!special.empty()) add(special[0],true);
 for(auto id:hierarchyRefs(graph,clip,"trackGroups",15)) add(id,false);
 return rows;
}
