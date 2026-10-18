#pragma once
inline bool executeReferenceSessionFloatPlay(Graph&graph,const ChannelBinding&binding,int32_t tick,SessionAttributeChanges*changes=nullptr,bool force=false,const std::vector<std::pair<bool,bool>>*infinite=nullptr){
 if(binding.mode!=3||!binding.destination.present()||binding.destination.scalarType!=3||binding.log==0xffffffff)return false;
 if(binding.layers.size()>1&&!infinite)return false;
 const auto&log=graph.elements.at(binding.log);if(graph.strings[log.type]!="DmeFloatLog")return false;
 const bool hasCurveInfo=!hierarchyRefs(graph,log,"curveinfo",1).empty();
 FloatLogDefault defaults(graph,log);
 const bool empty=sessionFloatLogEmpty(graph,binding.layers);auto path=channelPlayPath(true,true,force,empty,defaults.enabled,binding.source.present());float value;
 if(path==ChannelPlayPath::CopySource){if(binding.source.scalarType!=3)return false;auto*source=runtimeAttribute(graph,binding.source.element,binding.source.attribute);if(!source)throw std::runtime_error("missing playback fallback source");value=wireFloat(source->values.at(binding.source.index));}
 else if(binding.layers.empty())value=0.0f;
 else {
  int32_t selected=0;
  if(infinite)selected=selectLogLayer(sessionLogBounds(graph,binding.layers,*infinite),tick);
  const auto&layer=graph.elements.at(binding.layers.at(selected));KeyTrack track(graph,layer);
  if(hasCurveInfo){auto sampler=bindGraphFloatCurveSampler(graph,binding.log,binding.layers.at(selected));value=sampler.sample(tick);}
  else if(track.times->values.empty())value=defaults.emptyLayerValue();
  else {FloatKeySampler sampler(graph,layer);value=sampler.sample(tick);}
 }
 runtimeWriteFloat(graph,binding.destination.element,binding.destination.attribute,binding.destination.index,value,changes);return true;
}
