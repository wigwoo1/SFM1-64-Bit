#pragma once
#include "clip_hierarchy.h"
#include <map>
struct SessionMaterialParameterCandidate {uint8_t type;std::vector<Value>values;};
struct SessionMaterialOverrideCandidate {uint32_t element;std::string material;std::map<std::string,SessionMaterialParameterCandidate>parameters;};
inline std::string normalizeSessionMaterialNameCandidate(std::string name){
 for(char&c:name){if(c=='\\')c='/';else if(c>='A'&&c<='Z')c=char(c-'A'+'a');}
 if(name.size()>4&&name.substr(name.size()-4)==".vmt")name.resize(name.size()-4);
 if(name.empty()||name.front()=='/'||name.find(':')!=std::string::npos)throw std::runtime_error("session material identity");
 size_t first=0;while(first<=name.size()){auto end=name.find('/',first);auto part=name.substr(first,end==std::string::npos?end:end-first);if(part.empty()||part=="."||part=="..")throw std::runtime_error("session material identity component");if(end==std::string::npos)break;first=end+1;}return name;
}
inline std::vector<SessionMaterialOverrideCandidate>readSessionMaterialOverridesCandidate(const Graph&g,uint32_t model){
 const auto&e=g.elements.at(model);if(g.strings.at(e.type)!="DmeGameModel")throw std::runtime_error("material overrides require model");
 auto*a=namedAttribute(g,e,"materials");if(!a)return {};if(a->type!=15)throw std::runtime_error("model materials schema");
 std::vector<SessionMaterialOverrideCandidate>result;std::map<std::string,bool>seen;
 for(const auto&reference:a->values){auto id=reference.reference;const auto&material=g.elements.at(id);if(g.strings.at(material.type)!="DmeMaterial")throw std::runtime_error("model material element type");auto*name=namedAttribute(g,material,"mtlName");if(!name||name->type!=5||name->values.size()!=1)throw std::runtime_error("material mtlName schema");SessionMaterialOverrideCandidate record{id,normalizeSessionMaterialNameCandidate(name->values[0].text),{}};if(!seen.emplace(record.material,true).second)throw std::runtime_error("duplicate model material override");for(const auto&parameter:material.attributes){auto key=g.strings.at(parameter.name);if(key.empty()||key[0]!='$')continue;for(char&c:key)if(c>='A'&&c<='Z')c=char(c-'A'+'a');unsigned scalar=parameter.type>=15?parameter.type-14:parameter.type;if(scalar==1)throw std::runtime_error("element-valued material parameter unsupported");if(!record.parameters.emplace(key,SessionMaterialParameterCandidate{parameter.type,parameter.values}).second)throw std::runtime_error("duplicate material parameter");}result.push_back(std::move(record));}return result;
}
// Exact binary key, not a lossy float-to-string conversion or a hash.
inline std::string sessionMaterialOverrideKeyCandidate(const std::vector<SessionMaterialOverrideCandidate>&overrides){
 std::map<std::string,const SessionMaterialOverrideCandidate*>sorted;for(const auto&record:overrides)if(!sorted.emplace(record.material,&record).second)throw std::runtime_error("duplicate override key material");std::string key;auto count=[&](size_t n){if(n>UINT32_MAX)throw std::runtime_error("override key length");for(unsigned i=0;i<4;++i)key.push_back(char(uint32_t(n)>>(8*i)));};auto text=[&](const std::string&value){count(value.size());key.append(value);};count(sorted.size());for(const auto&entry:sorted){text(entry.first);count(entry.second->parameters.size());for(const auto&parameter:entry.second->parameters){text(parameter.first);key.push_back(char(parameter.second.type));count(parameter.second.values.size());for(const auto&value:parameter.second.values){text(value.text);count(value.wire.size());key.append(value.wire.begin(),value.wire.end());}}}return key;
}
