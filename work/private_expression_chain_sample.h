#pragma once
#include "session_expression_runtime.h"
struct PrivateExpressionChainSampleCandidate {
 bool playSuccess=false;
 float input=0;
 SessionExpressionSample expression;
};
// Tick is log-local. Admission/stable-read guards and native handle validation
// belong to the playback scheduler. No graph writes or notifications occur.
inline PrivateExpressionChainSampleCandidate evaluatePrivateExpressionChainInputCandidate(const Graph&graph,const ChannelBinding&play,uint32_t expression,float input){
 if(play.destination.element!=expression||play.destination.scalarType!=3||play.destination.index!=0||play.destination.attribute=="result")throw std::runtime_error("invalid private expression chain endpoint");
 PrivateExpressionChainSampleCandidate result;
 result.playSuccess=true;result.input=input;
 {
  const auto*attribute=namedAttribute(graph,graph.elements.at(expression),play.destination.attribute.c_str());
  if(!attribute||attribute->type!=3||attribute->values.size()!=1)throw std::runtime_error("invalid private chain input");
  const float current=wireFloat(attribute->values.at(0));
  // Ordered-equal writes are suppressed, including opposite signed zeros.
  // Evaluate with the value the serial channel commit would actually retain.
  if(!floatMutationNeeded(current,result.input))result.input=current;
  result.expression=sampleSessionExpression(graph,expression,{{play.destination.attribute,result.input}});
 }
 return result;
}
inline PrivateExpressionChainSampleCandidate samplePrivateExpressionChainCandidate(const Graph&graph,const ChannelBinding&play,uint32_t expression,int32_t tick){
 if(play.destination.element!=expression||play.destination.scalarType!=3||play.destination.index!=0||play.destination.attribute=="result")throw std::runtime_error("invalid private expression chain endpoint");
 float input=0;
 if(!sampleSessionFloatPlayCandidate(graph,play,tick,input))return {};
 return evaluatePrivateExpressionChainInputCandidate(graph,play,expression,input);
}
