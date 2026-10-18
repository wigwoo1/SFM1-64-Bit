#pragma once
inline bool executeReferenceSessionVectorPlay(Graph&g,const ChannelBinding&binding,int32_t tick,SessionAttributeChanges*changes=nullptr,const std::vector<std::pair<bool,bool>>*infinite=nullptr,const std::array<float,3>*disabledEdgeValue=nullptr){
 if(binding.mode!=3||!binding.destination.present()||binding.destination.scalarType!=10||binding.log==0xffffffff||binding.layers.empty())return false;
 if(binding.layers.size()>1&&!infinite)return false;
 const auto&log=g.elements.at(binding.log);if(g.strings.at(log.type)!="DmeVector3Log")return false;const bool curved=!hierarchyRefs(g,log,"curveinfo",1).empty();if(curved&&!disabledEdgeValue)return false;
 int32_t selected=infinite?selectLogLayer(sessionLogBounds(g,binding.layers,*infinite),tick):0;
 if(selected<0)return false;
 auto id=binding.layers.at(selected);KeyTrack track(g,g.elements.at(id));if(track.times->values.empty()&&!curved)return false;
 if(curved){auto sampler=bindGraphVectorCurveSampler(g,binding.log,id,*disabledEdgeValue);runtimeWriteVector(g,binding.destination,sampler.sample(tick),changes);return true;}
 auto sampler=bindGraphVectorKeyViewCandidate(g,binding.log,id);runtimeWriteVector(g,binding.destination,sampler.sample(tick),changes);return true;
}
