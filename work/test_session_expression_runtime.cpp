#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "schedule_traversal.h"
#include "channel_dependencies.h"
#include "expression_dependencies.h"
#include "session_expression_runtime.h"
#include "private_expression_chain_sample.h"
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("usage: test_session_expression_runtime session.dmx");auto graph=loadGraph(argv[1]);auto roots=hierarchyRefs(graph,graph.elements.at(0),"activeClip",1);ClipHierarchy hierarchy(graph,roots.at(0));
 for(auto channel:hierarchy.channels){ChannelBinding binding(graph,channel);
  if(binding.mode!=1||!binding.source.present()||!binding.destination.present()||binding.source.attribute!="result"||binding.source.scalarType!=3||binding.destination.scalarType!=3)continue;
  uint32_t expression=binding.source.element;const auto&element=graph.elements.at(expression);if(graph.strings[element.type]!="DmeExpressionOperator")continue;
  auto*formula=runtimeAttribute(graph,expression,"expr");if(!formula||formula->values.at(0).text!="lerp(value, lo, hi)")continue;
  auto*input=runtimeAttribute(graph,expression,"value");auto*low=runtimeAttribute(graph,expression,"lo");auto*high=runtimeAttribute(graph,expression,"hi");if(!input||!low||!high||wireFloat(low->values.at(0))==wireFloat(high->values.at(0)))continue;
  std::vector<AttributeOperation> operations={channelOperation(channel,binding),expressionOperation(graph,expression)};
  auto nodes=attributeDependencies(operations);auto schedule=traverseSchedule(nodes,{0,1});if(schedule.cycle||schedule.order.size()!=2||schedule.order[0]!=expression||schedule.order[1]!=channel)throw std::runtime_error("chain ordering failed");
  for(float changed:{0.25f,0.75f,0.5f}){
   const float oldInput=wireFloat(input->values.at(0));
   const auto privateSample=sampleSessionExpression(graph,expression,{{"value",changed}});
   if(wireFloat(input->values.at(0))!=oldInput)throw std::runtime_error("private sampling wrote input");
   setFloat(*input,0,changed);float expected=expressionLerp(changed,wireFloat(low->values[0]),wireFloat(high->values[0]));
   if(!privateSample.success||privateSample.result!=expected)throw std::runtime_error("private input sample mismatch");
   // Poison both outputs so a missing operation cannot pass by stale values.
   setFloat(*runtimeAttribute(graph,expression,"result"),0,12345);
   setFloat(*runtimeAttribute(graph,binding.destination.element,binding.destination.attribute),binding.destination.index,54321);
   const auto sampled=sampleSessionExpression(graph,expression);
   if(!sampled.success||sampled.result!=expected||wireFloat(runtimeAttribute(graph,expression,"result")->values.at(0))!=12345||wireFloat(runtimeAttribute(graph,binding.destination.element,binding.destination.attribute)->values.at(binding.destination.index))!=54321)throw std::runtime_error("read-only expression sampling failed");
   for(auto operation:schedule.order){if(operation==expression){if(!executeSessionExpression(graph,operation))throw std::runtime_error("expression failure");}else if(!executeSessionFloatCopy(graph,binding))throw std::runtime_error("copy failure");}
   auto actual=wireFloat(runtimeAttribute(graph,binding.destination.element,binding.destination.attribute)->values.at(binding.destination.index));if(actual!=expected)throw std::runtime_error("downstream value mismatch");
  }
  unsigned privateChains=0;
  for(auto upstream:hierarchy.channels){ChannelBinding play(graph,upstream);if(play.mode!=3||play.destination.element!=expression||play.destination.attribute!="value"||play.destination.scalarType!=3)continue;
   auto serialGraph=graph;
   for(int32_t tick:{-10000,0,10000}){
    const float unchanged=wireFloat(input->values.at(0));
    const auto sample=samplePrivateExpressionChainCandidate(graph,play,expression,tick);
    if(!sample.playSuccess||!sample.expression.success||wireFloat(input->values.at(0))!=unchanged)throw std::runtime_error("private chain sampling failed");
    if(!executeSessionFloatPlay(serialGraph,play,tick)||!executeSessionExpression(serialGraph,expression)||wireFloat(runtimeAttribute(serialGraph,expression,"result")->values.at(0))!=sample.expression.result||wireFloat(runtimeAttribute(serialGraph,expression,"value")->values.at(0))!=sample.input)throw std::runtime_error("private chain serial mismatch");
   }++privateChains;break;
  }
  if(privateChains!=1)throw std::runtime_error("private chain fixture not exercised");
  std::cout<<"private_chain_serial_checks=3 invalid_private_inputs=3\n";
  for(const std::string name:{"result","expr","missing_private_input"}){bool rejected=false;try{sampleSessionExpression(graph,expression,{{name,1.0f}});}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("invalid private input accepted");}
  const auto savedFormula=formula->values[0].text;formula->values[0].text="(";
  setFloat(*runtimeAttribute(graph,expression,"result"),0,12345);
  const auto failed=sampleSessionExpression(graph,expression);
  if(failed.success||wireFloat(runtimeAttribute(graph,expression,"result")->values.at(0))!=12345)throw std::runtime_error("failed sample mutated output");
  SessionAttributeChanges changes;
  if(executeSessionExpression(graph,expression,&changes)||wireFloat(runtimeAttribute(graph,expression,"result")->values.at(0))!=failed.result||changes.attributes!=std::vector<std::pair<uint32_t,std::string>>{{expression,"result"}})throw std::runtime_error("failed evaluation commit changed");
  formula->values[0].text=savedFormula;
  std::cout<<"expression="<<expression<<" channel="<<channel<<" destination="<<binding.destination.element<<":"<<binding.destination.attribute<<" scheduled_input_changes=3 downstream_checks=3 immutable_samples=3 failed_sample_and_commit=1 scope=in_memory_float_copy_chain\n";return 0;
 }
 throw std::runtime_error("no suitable real expression/copy chain found");
 }catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
