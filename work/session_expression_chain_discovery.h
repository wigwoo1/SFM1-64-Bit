#pragma once
#include "session_float_expression_playback.h"
struct SessionExpressionChainCandidate {uint32_t play,expression,copy;std::vector<uint32_t>frames;};
struct SessionExpressionChainDiscoveryCandidate {std::vector<SessionExpressionChainCandidate>chains;std::vector<std::pair<uint32_t,std::string>>invalidChannels;size_t channels=0,ambiguousPaths=0;};
inline SessionExpressionChainDiscoveryCandidate discoverSessionExpressionChainsCandidate(const Graph&graph,uint32_t root){
 SessionExpressionChainDiscoveryCandidate result;auto paths=channelTimePaths(graph,root);
 std::vector<std::pair<uint32_t,ChannelBinding>>bindings;
 for(const auto&item:paths){++result.channels;try{bindings.emplace_back(item.first,ChannelBinding(graph,item.first));}catch(const std::exception&e){result.invalidChannels.emplace_back(item.first,e.what());}}
 std::map<uint32_t,std::vector<uint32_t>>copies;
 for(const auto&item:bindings){const auto&b=item.second;if(b.mode==1&&b.source.present()&&b.destination.present()&&b.source.attribute=="result"&&b.source.scalarType==3&&b.destination.scalarType==3)copies[b.source.element].push_back(item.first);}
 for(const auto&item:bindings){const auto&b=item.second;if(b.mode!=3||!b.destination.present()||b.destination.attribute!="value"||b.destination.scalarType!=3||b.log==0xffffffff||b.layers.size()>1)continue;
  const auto expression=b.destination.element;if(graph.strings.at(graph.elements.at(expression).type)!="DmeExpressionOperator")continue;
  auto found=copies.find(expression);if(found==copies.end())continue;
  if(paths.at(item.first).size()!=1){++result.ambiguousPaths;continue;}
  for(auto copy:found->second)result.chains.push_back({item.first,expression,copy,paths.at(item.first).at(0)});
 }
 return result;
}

inline std::map<uint32_t,std::vector<uint32_t>> discoverSessionTypedPlayPathsCandidate(const Graph&graph,uint32_t root){
 std::map<uint32_t,std::vector<uint32_t>>result;for(const auto&item:channelTimePaths(graph,root)){ChannelBinding binding(graph,item.first);if(binding.mode!=3||!binding.destination.present()||(binding.destination.scalarType!=10&&binding.destination.scalarType!=13))continue;if(item.second.size()!=1)throw std::runtime_error("ambiguous typed playback path");result.emplace(item.first,item.second.at(0));}return result;
}

inline std::map<uint32_t,std::vector<uint32_t>> discoverSessionAdditionalPlayPathsCandidate(const Graph&graph,uint32_t root,const std::vector<SessionExpressionChainCandidate>&chains){
 std::set<uint32_t>covered;for(const auto&chain:chains){covered.insert(chain.play);covered.insert(chain.copy);}
 std::map<uint32_t,std::vector<uint32_t>>result;for(const auto&item:channelTimePaths(graph,root)){if(covered.count(item.first))continue;ChannelBinding b(graph,item.first);if(b.mode!=3||!b.destination.present()||(b.destination.scalarType!=3&&b.destination.scalarType!=10&&b.destination.scalarType!=13))continue;if(item.second.size()!=1)throw std::runtime_error("ambiguous additional playback path");result.emplace(item.first,item.second.at(0));}return result;
}

inline std::vector<uint32_t>discoverSessionColorCopyChannelsCandidate(const Graph&graph,uint32_t root){std::vector<uint32_t>result;for(const auto&item:channelTimePaths(graph,root)){ChannelBinding b(graph,item.first);if(b.mode==1&&b.source.present()&&b.destination.present()&&b.source.scalarType==8&&b.destination.scalarType==8)result.push_back(item.first);}return result;}
