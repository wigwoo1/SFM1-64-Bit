#pragma once
// Attribute identity excludes array index, matching runtime attribute pointers.
using AttributeIdentity=std::pair<uint32_t,std::string>;
inline std::vector<ScheduleNode> channelDependencies(const std::vector<uint32_t>&ids,const std::vector<ChannelBinding>&bindings){
 if(ids.size()!=bindings.size())throw std::runtime_error("channel id/binding mismatch");
 std::map<AttributeIdentity,std::vector<uint32_t>> consumers;std::vector<ScheduleNode> nodes;
 for(size_t i=0;i<bindings.size();++i){auto&b=bindings[i];nodes.push_back({ids[i],{}});if(b.mode!=0&&b.mode!=3&&b.source.present())consumers[{b.source.element,b.source.attribute}].push_back(static_cast<uint32_t>(i));}
 for(size_t i=0;i<bindings.size();++i){auto&b=bindings[i];if(b.mode==0||!b.destination.present())continue;AttributeIdentity dest={b.destination.element,b.destination.attribute};if((b.mode==1||b.mode==2)&&b.source.present()&&dest==AttributeIdentity{b.source.element,b.source.attribute})continue;auto found=consumers.find(dest);if(found!=consumers.end())nodes[i].edgeGroups.push_back(found->second);}
 return nodes;
}
