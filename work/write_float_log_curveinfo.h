#pragma once
#include "coordinated_reference_write.h"
#include "runtime_curveinfo_default.h"
#include "session_reference_propagation.h"
#include "recovered_curveinfo_classes.h"
#include "refresh_mutating_float_curves.h"
#include "float_layer_notifications.h"
// Native float-log slot 0 (0x10574580) dispatches curveinfo changes to
// 0x1054D890. Invalid native layer pointers are rejected rather than dereferenced.
inline size_t dispatchFloatLogCurveInfoChanged(RuntimeFloatLayerState&state,uint32_t log,uint32_t changed,bool refreshSuppressed=false){
 auto&r=state.runtime;auto e=r.elements.resolve(r.graph,log);
 if(!e||r.graph.strings.at(e->type)!="DmeFloatLog")throw std::runtime_error("invalid curveinfo callback log");
 if(changed!=r.attributes.find(log,"curveinfo"))return 0;
 auto layers=r.attributes.resolve(r.graph,r.elements,r.attributes.find(log,"layers"));
 if(!layers||layers->type!=15)throw std::runtime_error("invalid curveinfo callback layers");
 const auto count=layers->values.size();size_t refreshed=0;
 for(size_t i=0;i<count;++i){auto ref=layers->values.at(i).reference;
  if(ref>=r.graph.elements.size())throw std::runtime_error("unresolved curveinfo callback layer");
  auto layer=r.elements.graphHandles.at(ref);
  if(!refreshRuntimeLayerCurveTypes(state,layer,refreshSuppressed))throw std::runtime_error("unconstructed curveinfo callback layer");
  ++refreshed;
 }return refreshed;
}
template<class Suppressed>
bool writeFloatLogCurveInfo(SessionRuntime&r,MutationCoordinator&m,RuntimeFloatLayerState&state,uint32_t log,uint32_t target,bool preparationSuppressed,Suppressed propagationSuppressed,bool refreshSuppressed=false,uint32_t*frameworkDirty=nullptr,RuntimeIntegerUndoCapture*capture=nullptr){
 if(&state.runtime!=&r)throw std::runtime_error("curveinfo runtime mismatch");
 auto e=r.elements.resolve(r.graph,log);if(!e||r.graph.strings.at(e->type)!="DmeFloatLog")return false;
 if(capture&&capture->retention&&&capture->retention->runtime!=&r)throw std::runtime_error("scalar capture runtime mismatch");
 if(!coordinatedScalarReferenceWriteChecked(r,m,log,"curveinfo",target,preparationSuppressed,[&](uint32_t candidate){
  auto element=r.elements.resolve(r.graph,candidate);const auto&type=r.graph.strings.at(element->type);
  if(recoveredCurveInfoClass(type))return true;
  if(type=="DmeFloatLog"||type=="DmeFloatLogLayer"||type=="DmeChannel")return false;
  throw std::runtime_error("unrecovered float curveinfo target ancestry: "+type);
 },[&](){if(capture&&capture->retention&&capture->enabled(r,log))capture->journal.append(std::make_shared<RuntimeScalarReferenceUndoRecord>(captureScalarReferenceRecord(r,*capture->retention,log,"curveinfo",target)));}))return false;
 auto attr=r.attributes.find(log,"curveinfo");auto identity=r.attributes.identity(r.graph,r.elements,attr);auto&owner=m.owners.at(log);
 uint32_t localDirty=0;auto&dirty=frameworkDirty?*frameworkDirty:localDirty;
 if((owner.attributeFlags.at(identity)&0x100)&&(owner.state&4))dispatchMutatingFloatLogCurves(state,m,log,attr,dirty,refreshSuppressed,preparationSuppressed,[&](unsigned slot,uint32_t attribute,int32_t first,int32_t last){notifyRecoveredFloatLayer(r,slot,attribute,first,last);},propagationSuppressed,capture);
 propagateSessionReferences(r,m,log,3,propagationSuppressed);dirty|=0x1000;return true;
}






