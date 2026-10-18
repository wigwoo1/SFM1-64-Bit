#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "session_expression_runtime.h"
#include "log_layer_runtime_state.h"
#include "session_log_layer_remove.h"
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("session path required");auto g=loadGraph(argv[1]);
 for(uint32_t id=0;id<g.elements.size();++id){
  if(g.strings[g.elements[id].type]!="DmeChannel")continue;ChannelBinding b(g,id);
  if(b.mode!=3||!b.destination.present()||b.destination.scalarType!=3||b.log==0xffffffff||b.layers.size()!=1)continue;
  if(!hierarchyRefs(g,g.elements[b.log],"curveinfo",1).empty())continue;
  KeyTrack original(g,g.elements[b.layers[0]]);if(original.times->values.empty())continue;
  int32_t tick=wireInt(original.times->values.front());
  auto overlay=g.elements[b.layers[0]];uint32_t second=uint32_t(g.elements.size());g.elements.push_back(overlay);b.layers.push_back(second);
  auto*v=runtimeAttribute(g,second,"values");for(size_t i=0;i<v->values.size();++i)setFloat(*v,i,123.25f);
  LogLayerRuntimeState state;state.construct(b.layers[0]);state.construct(second);
  if(state.forLayers(b.layers)!=std::vector<std::pair<bool,bool>>{{false,false},{false,false}})throw std::runtime_error("fresh layer flags mismatch");
  state.setInfinite(second,true,true);auto flags=state.forLayers(b.layers);
  state.attach(second,b.log);if(state.owners.at(second)!=b.log||state.forLayers(b.layers)!=flags)throw std::runtime_error("attachment changed boundary flags");
  auto check=[&](float expected,bool force=false){if(!executeSessionFloatPlay(g,b,tick,nullptr,force,&flags)||wireFloat(runtimeAttribute(g,b.destination.element,b.destination.attribute)->values[b.destination.index])!=expected)throw std::runtime_error("multilayer playback mismatch");};
  check(123.25f); // Last eligible layer takes precedence.
  runtimeAttribute(g,second,"times")->values.clear();runtimeAttribute(g,second,"values")->values.clear();
  auto*enabled=runtimeAttribute(g,b.log,"usedefaultvalue");enabled->values[0].wire[0]=1;setFloat(*runtimeAttribute(g,b.log,"defaultvalue"),0,77.5f);
  check(77.5f); // Empty infinite overlay with a nonempty base uses owner default.
  enabled->values[0].wire[0]=0;check(0.0f);
  state.copy(b.layers[0],second);if(state.forLayers(b.layers)!=std::vector<std::pair<bool,bool>>{{true,true},{true,true}})throw std::runtime_error("copied layer flags mismatch");
  if(state.owners.at(b.layers[0])!=0xffffffff||state.owners.at(second)!=b.log)throw std::runtime_error("content copy changed owner");
  auto*logLayers=runtimeAttribute(g,b.log,"layers");auto newRef=logLayers->values.at(0);newRef.reference=second;logLayers->values.push_back(newRef);
  unsigned released=0;auto release=[&](uint32_t ref){if(ref!=second||logLayers->values.size()!=2)throw std::runtime_error("reference release ordering mismatch");++released;};
  auto elementCount=g.elements.size();if(!removeSessionLogLayerReference(g,b.log,release)||released!=1||logLayers->values.size()!=1||g.elements.size()!=elementCount)throw std::runtime_error("layer reference removal mismatch");
  b.layers.clear();flags.clear();enabled->values[0].wire[0]=1;check(0.0f);check(0.0f,true);
  std::cout<<"multilayer_and_zero_layer_checks=5 runtime_lifecycle_checks=5 channel="<<id<<'\n';return 0;
 }
 throw std::runtime_error("no usable float channel");
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
