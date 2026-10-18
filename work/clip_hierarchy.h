#pragma once
#include <set>
inline const Attribute* namedAttribute(const Graph&g,const Element&e,const char*name){const Attribute*result=nullptr;for(auto&a:e.attributes)if(g.strings.at(a.name)==name){if(result)throw std::runtime_error("duplicate hierarchy attribute");result=&a;}return result;}
inline std::vector<uint32_t> hierarchyRefs(const Graph&g,const Element&e,const char*name,uint8_t type){auto*a=namedAttribute(g,e,name);if(!a)return {};if(a->type!=type||(type==1&&a->values.size()!=1))throw std::runtime_error("invalid hierarchy reference field");std::vector<uint32_t> out;for(auto&v:a->values){if(v.reference==0xffffffff)continue;if(v.reference>=g.elements.size())throw std::runtime_error("unresolved hierarchy reference");out.push_back(v.reference);}return out;}
inline uint32_t hierarchySingleReference(const Graph&g,const Element&e,const char*name){
 const auto*a=namedAttribute(g,e,name);if(!a)return 0xffffffff;
 if(a->type!=1||a->values.size()!=1)throw std::runtime_error("invalid hierarchy reference field");
 auto ref=a->values[0].reference;if(ref!=0xffffffff&&ref>=g.elements.size())throw std::runtime_error("unresolved hierarchy reference");return ref;
}
struct ClipHierarchy {
 std::set<uint32_t> nodes,clips,channels;std::map<uint32_t,uint32_t> frames;
 explicit ClipHierarchy(const Graph&g,uint32_t root){std::set<uint32_t> visiting;
  auto walk=[&](auto&&self,uint32_t index)->void{
   if(index>=g.elements.size())throw std::runtime_error("bad hierarchy root");if(visiting.count(index))throw std::runtime_error("cyclic clip hierarchy");if(nodes.count(index))return;
   visiting.insert(index);auto&e=g.elements[index];auto&t=g.strings.at(e.type);
   auto follow=[&](const char*n,uint8_t kind){for(auto ref:hierarchyRefs(g,e,n,kind))self(self,ref);};
   if(t=="DmeTrackGroup")follow("tracks",15);
   else if(t=="DmeTrack")follow("children",15);
   else if(t=="DmeChannel")channels.insert(index);
   else if(t.find("Clip")!=std::string::npos){clips.insert(index);auto f=hierarchyRefs(g,e,"timeFrame",1);if(f.size()!=1||g.strings.at(g.elements[f[0]].type)!="DmeTimeFrame")throw std::runtime_error("clip lacks time frame");frames[index]=f[0];follow("trackGroups",15);if(t=="DmeFilmClip")follow("subClipTrackGroup",1);if(t=="DmeChannelsClip")follow("channels",15);}
   else throw std::runtime_error("unexpected clip hierarchy element");
   visiting.erase(index);nodes.insert(index);
  };walk(walk,root);
 }
};
