#pragma once
#include "registered_operator_order.h"
#include "operator_root_selection.h"
// Attribute table order is supplied explicitly; registration order is not assumed.
// Callbacks must preserve registration storage and attribute identity lifetimes.
// Installed SWIG wrappers identify virtual offsets0/4 as IsOperatorDirty and
// ClearOperatorDirtyFlag. Only operators dirty at selection time are cleared.
template<class IsDirty,class ClearDirty,class Flags,class Assign>
OperatorDependencyOrder buildRegisteredOperators(const OperatorAttributeRegistration&r,const std::vector<size_t>&attributeTableOrder,IsDirty isDirty,ClearDirty clearDirty,Flags flags,Assign assign){
 std::vector<bool> activeStates(r.operators.size());
 for(size_t i=0;i<r.operators.size();++i)if(isDirty(r.operators[i].identity)){activeStates[i]=true;clearDirty(r.operators[i].identity);}
 std::vector<OperatorRootAttribute> attributes;
 for(auto index:attributeTableOrder){const auto&a=r.attributes.at(index);attributes.push_back({flags(a.identity),a.excluded,a.associatedOperators});}
 auto roots=selectOperatorRoots(activeStates,attributes);
 for(size_t i=0;i<attributes.size();++i)flags(r.attributes.at(attributeTableOrder[i]).identity)=attributes[i].flags;
 auto result=orderRegisteredOperators(r,roots);
 for(size_t i=0;i<result.identities.size();++i)assign(result.identities[i],i);
 return result;
}
