#pragma once
#include "runtime_curveinfo_default.h"
#include "mutate_runtime_integer_array.h"
#include "recorded_integer_mutation.h"
// Recovered 1054C980 sequence: clear, snapshot key count, then look up the
// current owner default again for each append. Individual gate failures are
// ignored by the native caller; this is not an atomic vector replacement.
template<class Notify,class Suppressed>
bool refreshMutatingFloatLayerCurves(RuntimeFloatLayerState&state,MutationCoordinator&m,uint32_t layer,uint32_t&dirty,bool refreshSuppressed,bool preparationSuppressed,Notify notify,Suppressed suppressed,RuntimeIntegerUndoCapture*capture=nullptr){
 if(!state.resolve(layer))return false;if(refreshSuppressed)return true;
 auto&r=state.runtime;const bool hasDefault=runtimeLayerDefaultCurveType(state,layer).has_value();
 if(capture)clearRecordedIntegerArray(r,m,*capture,layer,"curvetypes",dirty,preparationSuppressed,notify,suppressed);
 else clearRuntimeIntegerArray(r,m,layer,"curvetypes",dirty,preparationSuppressed,notify,suppressed);
 if(!hasDefault)return true;
 auto times=r.attributes.resolve(r.graph,r.elements,r.attributes.find(layer,"times"));if(!times||times->type!=21)throw std::runtime_error("invalid curve rebuild times");const auto count=times->values.size();
 for(size_t i=0;i<count;++i){auto value=runtimeLayerDefaultCurveType(state,layer);if(!value)throw std::runtime_error("curveinfo disappeared during rebuild");if(capture)appendRecordedIntegerArray(r,m,*capture,layer,"curvetypes",*value,dirty,preparationSuppressed,notify,suppressed);else appendRuntimeIntegerArray(r,m,layer,"curvetypes",*value,dirty,preparationSuppressed,notify,suppressed);}return true;
}
template<class Notify,class Suppressed>
size_t dispatchMutatingFloatLogCurves(RuntimeFloatLayerState&state,MutationCoordinator&m,uint32_t log,uint32_t changed,uint32_t&dirty,bool refreshSuppressed,bool preparationSuppressed,Notify notify,Suppressed suppressed,RuntimeIntegerUndoCapture*capture=nullptr){
 auto&r=state.runtime;auto e=r.elements.resolve(r.graph,log);if(!e||r.graph.strings.at(e->type)!="DmeFloatLog")throw std::runtime_error("invalid mutating curve callback log");
 if(changed!=r.attributes.find(log,"curveinfo"))return 0;
 auto layers=r.attributes.resolve(r.graph,r.elements,r.attributes.find(log,"layers"));if(!layers||layers->type!=15)throw std::runtime_error("invalid mutating curve callback layers");const auto count=layers->values.size();
 for(size_t i=0;i<count;++i){layers=r.attributes.resolve(r.graph,r.elements,r.attributes.find(log,"layers"));auto ref=layers->values.at(i).reference;if(ref>=r.graph.elements.size())throw std::runtime_error("unresolved mutating curve callback layer");
 if(!refreshMutatingFloatLayerCurves(state,m,r.elements.graphHandles.at(ref),dirty,refreshSuppressed,preparationSuppressed,notify,suppressed,capture))throw std::runtime_error("unconstructed mutating curve callback layer");}return count;
}

