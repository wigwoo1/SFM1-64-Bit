#pragma once
#include <cstdint>
// Protocol of immediate context slot9 (100265E0) and its base helper10025DD0.
// Opaque IDs are pointer-width. Ops supplies live context/default/material and
// renderer services. No value snapshot, ownership, lock or renderer admission
// is added. Callbacks may change the context; reads intentionally occur again.
// Native preconditions apply: callbacks/identities must remain valid. Exceptions
// propagate; this does not invent a transaction or native failure recovery.
template<class Ops>inline void studioMaterialImmediateBindCandidate(uintptr_t supplied,uintptr_t proxyData,Ops&ops){
 uintptr_t material=supplied;
 if(!material){if(!ops.defaultMaterial())return;ops.nullDiagnostic(false);material=ops.defaultMaterial();}
 const auto diagnosticMaterial=ops.realTimeMaterial(material);
 if(ops.debugEnabled())ops.debugMaterial(diagnosticMaterial);
 material=supplied;
 if(!material){ops.nullDiagnostic(true);material=ops.defaultMaterial();}
 const auto selected=ops.realTimeMaterial(material);
 if(ops.currentMaterial()!=selected){
  if(!ops.precached(selected)){const auto name=ops.materialName(selected);ops.uncachedDiagnostic(name);ops.precache(selected);ops.refresh(selected);}
  ops.setMaterial(selected);
 }
 ops.setProxyData(proxyData);
 ops.flush();
 // Only exact zero executes proxies. The enum meaning/current SFM mode is
 // deliberately not inferred. Shader binding reads live state AFTER proxies.
 if(ops.threadMode()==0)ops.callProxies(ops.currentMaterial(),proxyData);
 ops.shaderBind(ops.currentMaterial());
}
