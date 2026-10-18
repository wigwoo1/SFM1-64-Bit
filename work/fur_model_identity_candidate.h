#pragma once
#include <optional>
inline std::string furModelAssetCandidate(const Graph&g,uint32_t model){
 const auto&e=g.elements.at(model);if(g.strings.at(e.type)!="DmeGameModel")throw std::runtime_error("fur target must be a game model");
 const Attribute*found=nullptr;for(const auto&a:e.attributes)if(g.strings.at(a.name)=="modelName"){if(found)throw std::runtime_error("duplicate model asset attribute");found=&a;}
 if(!found||found->type!=5||found->values.size()!=1||found->values[0].text.empty())throw std::runtime_error("fur target has no model asset");return found->values[0].text;
}
inline std::string furModelUuidCandidate(const Element&e){std::string result;const char*hex="0123456789abcdef";for(unsigned char c:e.id){result+=hex[c>>4];result+=hex[c&15];}return result;}
class FurModelIdentityCandidate{
 std::array<char,16>uuid_;std::string asset_;
public:
 FurModelIdentityCandidate(const Graph&g,uint32_t model):uuid_(g.elements.at(model).id),asset_(furModelAssetCandidate(g,model)){if(!resolve(g))throw std::runtime_error("fur model identity missing");}
 std::optional<uint32_t>resolve(const Graph&g)const{
  std::optional<uint32_t>found;for(uint32_t i=0;i<g.elements.size();++i)if(g.elements[i].id==uuid_){if(found)throw std::runtime_error("ambiguous fur model UUID");found=i;}
  if(found&&furModelAssetCandidate(g,*found)!=asset_)throw std::runtime_error("fur model asset changed");return found;
 }
};
