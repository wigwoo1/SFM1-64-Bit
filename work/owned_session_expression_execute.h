#pragma once
#include "session_expression_runtime.h"
// Explicit owned projection adapter. Not yet used by normal timeline playback.
// Installed expression element slot0 scalar-change callback is ret4/no-op.
// Generic projection writes still require callbacks for other derived classes.
// Failed parses still attempt to commit the parser result, as in the serial path.
template<class Projection>SessionExpressionSample executeOwnedSessionExpressionCandidate(Projection&projection,uint32_t element,int phase){
 const auto sample=sampleSessionExpression(projection.graph,element);
 const auto*output=namedAttribute(projection.graph,projection.graph.elements.at(element),"result");
 const bool changed=floatMutationNeeded(wireFloat(output->values.at(0)),sample.result);
 if(element>=projection.elementHandles.size()||!projection.scene.resolve(projection.elementHandles[element]))throw std::runtime_error("stale owned expression");
 if(!projection.setScalarFloat(element,"result",sample.result,phase)&&changed)throw std::runtime_error("owned expression result write rejected");
 return sample;
}
