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
#include "session_expression_chain_discovery.h"
#include "graph_exact_equal.h"
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("usage: test_session_play_expression session.dmx");SessionRuntime owned(loadGraph(argv[1]));auto&graph=owned.graph;auto roots=hierarchyRefs(graph,graph.elements[0],"activeClip",1);ClipHierarchy hierarchy(graph,roots.at(0));auto paths=channelTimePaths(graph,roots.at(0));if(paths.size()!=hierarchy.channels.size())throw std::runtime_error("time path coverage failure");
 auto discovered=discoverSessionExpressionChainsCandidate(graph,roots.at(0));if(discovered.channels!=hierarchy.channels.size()||!discovered.invalidChannels.empty()||discovered.chains.empty())throw std::runtime_error("chain discovery coverage mismatch");
 for(auto playId:hierarchy.channels){ChannelBinding play(graph,playId);if(play.mode!=3||!play.destination.present()||play.destination.attribute!="value"||play.destination.scalarType!=3||play.log==0xffffffff||play.layers.size()!=1)continue;
  uint32_t expression=play.destination.element;if(graph.strings[graph.elements[expression].type]!="DmeExpressionOperator")continue;
  auto*formula=runtimeAttribute(graph,expression,"expr");if(!formula||formula->values[0].text!="lerp(value, lo, hi)")continue;
  if(!hierarchyRefs(graph,graph.elements[play.log],"curveinfo",1).empty())continue;
  KeyTrack keys(graph,graph.elements[play.layers[0]]);if(keys.times->values.size()<2||wireFloat(keys.values->values.front())==wireFloat(keys.values->values.back()))continue;
  if(paths.at(playId).size()!=1)continue;const auto&frames=paths.at(playId)[0];bool unit=true;for(auto frame:frames)unit&=AnimationTimeFrame(graph,graph.elements[frame]).scale==1;if(!unit)continue;
  for(auto copyId:hierarchy.channels){ChannelBinding copy(graph,copyId);if(copy.mode!=1||copy.source.element!=expression||copy.source.attribute!="result"||!copy.destination.present()||copy.destination.scalarType!=3)continue;
   auto nodes=attributeDependencies({channelOperation(copyId,copy),expressionOperation(graph,expression),channelOperation(playId,play)});auto schedule=traverseSchedule(nodes,{0,1,2});if(schedule.cycle||schedule.order!=std::vector<uint32_t>{playId,expression,copyId})throw std::runtime_error("playback chain order mismatch");
   SessionChangeScheduler incremental({channelOperation(copyId,copy),expressionOperation(graph,expression),channelOperation(playId,play)});
   auto&runtime=owned.elements;auto&attributes=owned.attributes;RuntimeCachedFloatPlayBinding livePlay(graph,playId,runtime,attributes);RuntimeCachedFloatCopyBinding liveCopy(graph,copyId,runtime,attributes);uint32_t expressionHandle=runtime.graphHandles.at(expression);
   bool located=false;for(const auto&chain:discovered.chains)if(chain.play==playId&&chain.expression==expression&&chain.copy==copyId&&chain.frames==frames)located=true;if(!located)throw std::runtime_error("tested chain absent from discovery");
   SessionFloatExpressionPlaybackCandidate playback(owned,playId,expression,copyId,frames);
   unsigned recreations=0;
   auto recreate=[&](uint32_t element,const std::string&name){
    auto owner=runtime.graphHandles.at(element);auto old=attributes.find(owner,name);auto&storage=graph.elements.at(element).attributes;
    auto it=std::find_if(storage.begin(),storage.end(),[&](const Attribute&a){return graph.strings.at(a.name)==name;});if(it==storage.end())throw std::runtime_error("recreation fixture missing");auto saved=*it;
    if(!owned.removeAttribute(owner,name)||owned.removeAttribute(owner,name))throw std::runtime_error("chain attribute removal failed");
    SessionAttributeChanges missing;
    bool executed=name=="value"?executeRuntimeFloatPlay(graph,runtime,attributes,livePlay,0,&missing):executeRuntimeFloatCopy(graph,runtime,attributes,liveCopy,&missing);
    if(executed||!missing.attributes.empty())throw std::runtime_error("chain missing attribute accepted");
    storage.push_back(saved);auto fresh=attributes.bind(graph,runtime,owner,name);
    if(old==fresh||attributes.resolve(graph,runtime,old))throw std::runtime_error("chain stale attribute accepted");++recreations;
   };
   unsigned checks=0;for(auto index:{size_t(0),keys.times->values.size()-1,keys.times->values.size()/2}){
    int32_t tick=wireInt(keys.times->values[index]);size_t last=index;while(last+1<keys.times->values.size()&&wireInt(keys.times->values[last+1])==tick)++last;float sampled=wireFloat(keys.values->values[last]);
    int32_t rootTick=tick;for(auto frame=frames.rbegin();frame!=frames.rend();++frame){AnimationTimeFrame time(graph,graph.elements[*frame]);rootTick=wrapTicks(uint32_t(rootTick)-uint32_t(time.offset)+uint32_t(time.start));}
    int32_t local=channelLocalTime(graph,frames,rootTick,false);if(local!=tick)throw std::runtime_error("session-to-log time roundtrip failed");
    float expected=expressionLerp(sampled,wireFloat(runtimeAttribute(graph,expression,"lo")->values[0]),wireFloat(runtimeAttribute(graph,expression,"hi")->values[0]));
    recreate(expression,"value");recreate(expression,"result");recreate(copy.destination.element,copy.destination.attribute);
    setFloat(*runtimeAttribute(graph,expression,"value"),0,12345);setFloat(*runtimeAttribute(graph,expression,"result"),0,54321);setFloat(*runtimeAttribute(graph,copy.destination.element,copy.destination.attribute),copy.destination.index,99999);
    SessionAttributeChanges changes;
    auto run=[&](){for(auto id:schedule.order){bool success=id==playId?executeRuntimeFloatPlay(graph,runtime,attributes,livePlay,channelLocalTime(graph,frames,rootTick,false),&changes):id==expression?executeRuntimeExpression(graph,runtime,expressionHandle,&changes):executeRuntimeFloatCopy(graph,runtime,attributes,liveCopy,&changes);if(!success)throw std::runtime_error("operation not executed");}};
    run();if(changes.attributes!=std::vector<AttributeIdentity>{{expression,"value"},{expression,"result"},{copy.destination.element,copy.destination.attribute}})throw std::runtime_error("attribute change order mismatch");
    changes.attributes.clear();run();if(!changes.attributes.empty())throw std::runtime_error("unchanged playback emitted changes");
    setFloat(*runtimeAttribute(graph,expression,"value"),0,12345);setFloat(*runtimeAttribute(graph,expression,"result"),0,54321);setFloat(*runtimeAttribute(graph,copy.destination.element,copy.destination.attribute),copy.destination.index,99999);
    auto execute=[&](uint32_t id,SessionAttributeChanges&events){return id==playId?executeRuntimeFloatPlay(graph,runtime,attributes,livePlay,local,&events):id==expression?executeRuntimeExpression(graph,runtime,expressionHandle,&events):executeRuntimeFloatCopy(graph,runtime,attributes,liveCopy,&events);};
    if(incremental.step({playId},{},execute)!=schedule.order)throw std::runtime_error("dirty chain execution mismatch");
    if(incremental.step({playId},{},execute)!=std::vector<uint32_t>{playId})throw std::runtime_error("unchanged downstream was executed");
    setFloat(*runtimeAttribute(graph,expression,"value"),0,12345);setFloat(*runtimeAttribute(graph,expression,"result"),0,54321);setFloat(*runtimeAttribute(graph,copy.destination.element,copy.destination.attribute),copy.destination.index,99999);
    if(playback.step(rootTick,false)!=schedule.order||playback.step(rootTick,false)!=std::vector<uint32_t>{playId})throw std::runtime_error("reusable playback chain mismatch");
    float actual=wireFloat(runtimeAttribute(graph,copy.destination.element,copy.destination.attribute)->values.at(copy.destination.index));if(actual!=expected||wireFloat(runtimeAttribute(graph,expression,"value")->values[0])!=sampled)throw std::runtime_error("animated downstream mismatch");if(livePlay.destinationAttribute!=attributes.find(expressionHandle,"value")||liveCopy.sourceAttribute!=attributes.find(expressionHandle,"result")||liveCopy.destinationAttribute!=attributes.find(liveCopy.destinationHandle,copy.destination.attribute))throw std::runtime_error("chain caches did not refresh");++checks;
   }
   unsigned preflightChecks=0;
   for(auto retiredIndex:{playId,copyId,expression,copy.destination.element,play.log,play.layers.at(0),frames.at(0)}){
    SessionRuntime isolated(graph);SessionFloatExpressionPlaybackCandidate candidate(isolated,playId,expression,copyId,frames);
    isolated.elements.retire(isolated.elements.graphHandles.at(retiredIndex));
    auto before=isolated.graph;bool rejected=false;try{candidate.step(123,false);}catch(const std::runtime_error&){rejected=true;}
    if(!rejected||!graphExactEqualCandidate(before,isolated.graph))throw std::runtime_error("stale chain preflight mutated graph");++preflightChecks;
   }
   unsigned attributePreflightChecks=0;
   for(auto endpoint:std::vector<AttributeIdentity>{{expression,"value"},{expression,"result"},{expression,"expr"},{copy.destination.element,copy.destination.attribute}}){
    SessionRuntime isolated(graph);SessionFloatExpressionPlaybackCandidate candidate(isolated,playId,expression,copyId,frames);
    if(!isolated.removeAttribute(isolated.elements.graphHandles.at(endpoint.first),endpoint.second))throw std::runtime_error("preflight removal failed");
    auto before=isolated.graph;bool rejected=false;try{candidate.step(123,false);}catch(const std::runtime_error&){rejected=true;}
    if(!rejected||!graphExactEqualCandidate(before,isolated.graph))throw std::runtime_error("missing attribute preflight mutated graph");++attributePreflightChecks;
   }
   runtime.retire(expressionHandle);SessionAttributeChanges retiredEvents;
   if(executeRuntimeExpression(graph,runtime,expressionHandle,&retiredEvents)||executeRuntimeFloatPlay(graph,runtime,attributes,livePlay,0,&retiredEvents)||executeRuntimeFloatCopy(graph,runtime,attributes,liveCopy,&retiredEvents)||!retiredEvents.attributes.empty())throw std::runtime_error("retired expression chain executed");
   std::cout<<"root_clip="<<roots.at(0)<<" play="<<playId<<" expression="<<expression<<" copy="<<copyId<<" stale_chain_preflight_checks="<<preflightChecks<<" missing_attribute_preflight_checks="<<attributePreflightChecks<<" sampled_chain_checks="<<checks<<" attribute_recreation_checks="<<recreations<<" retired_operation_checks=3 time_frames="<<frames.size()<<" covered_channels="<<paths.size()<<" scope=cached_attribute_handles_root_time_unclamped_unit_scale_chain\n";return 0;
  }
 }throw std::runtime_error("no suitable animated chain");
 }catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}

