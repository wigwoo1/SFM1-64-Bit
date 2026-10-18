#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "schedule_traversal.h"
#include "channel_dependencies.h"
#include "expression_dependencies.h"
#include "session_expression_runtime.h"
#include "channel_clip_time.h"
#include "session_change_scheduler.h"
#include "runtime_channel_play.h"
#include "runtime_expression.h"
#include "session_runtime.h"
#include "session_timeline_document_candidate.h"
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("INPUT FRESH_OUTPUT required");SessionTimelineDocumentCandidate document(loadGraph(argv[1]));const auto&g=document.graph();
 for(uint32_t channel=0;channel<g.elements.size();++channel){if(g.strings.at(g.elements[channel].type)!="DmeChannel")continue;ChannelBinding binding(g,channel);if(binding.mode!=3||!binding.destination.present()||binding.destination.scalarType!=3||binding.layers.size()!=1)continue;auto layer=binding.layers[0];KeyTrack keys(g,g.elements[layer]);if(keys.times->values.empty())continue;if(!hierarchyRefs(g,g.elements[binding.log],"curveinfo",1).empty())continue;
 size_t key=0;int32_t tick=wireInt(keys.times->values[0]);while(key+1<keys.times->values.size()&&wireInt(keys.times->values[key+1])<=tick)++key;if(key+1>=keys.times->values.size())continue;const int64_t gap=int64_t(wireInt(keys.times->values[key+1]))-tick;if(gap<=1)continue;const int32_t movedTick=int32_t(int64_t(tick)+gap/2);float old=wireFloat(keys.values->values[key]);float replacement=old==0.375f?0.625f:0.375f;
 auto sample=[&](Graph graph,int32_t sampleTick,float expected){SessionRuntime runtime(std::move(graph));RuntimeCachedFloatPlayBinding live(runtime.graph,channel,runtime.elements,runtime.attributes);if(!executeRuntimeFloatPlay(runtime.graph,runtime.elements,runtime.attributes,live,sampleTick,nullptr,true))throw std::runtime_error("play failed");auto*a=runtimeAttribute(runtime.graph,binding.destination.element,binding.destination.attribute);if(!a||wireFloat(a->values.at(binding.destination.index))!=expected)throw std::runtime_error("edited key did not reach destination");};
 sample(document.graph(),tick,old);if(!document.setFloatKey(layer,key,movedTick,replacement))throw std::runtime_error("edit rejected");sample(document.graph(),movedTick,replacement);if(document.undo()!=2)throw std::runtime_error("joint edit undo failed");sample(document.graph(),tick,old);if(document.redo()!=2)throw std::runtime_error("joint edit redo failed");sample(document.graph(),movedTick,replacement);document.saveAs(argv[2]);sample(loadGraph(argv[2]),movedTick,replacement);
 std::cout<<"joint_key_playback_checks=6 channel="<<channel<<" layer="<<layer<<" key="<<key<<" old_tick="<<tick<<" moved_tick="<<movedTick<<" saved_reload=true\n";return 0;
 }throw std::runtime_error("no supported float playback channel");
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
