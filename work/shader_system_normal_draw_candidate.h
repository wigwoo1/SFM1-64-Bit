#pragma once
#include <cstdint>
// Normal/missing-snapshot paths of10037D40, not its debug/fallback dispatcher.
// Caller must establish that debug overrides are disabled and the mode is
// eligible for the normal shader. Ops owns valid live objects, snapshot bounds,
// logging services and storage. Raw field accessors must have no callbacks or
// side effects; service callbacks are explicit. No exception rollback is added.
// This is a synchronous protocol, not ownership,
// thread safety, a material-value snapshot or renderer admission.
struct ShaderSystemNormalDrawStateCandidate {
 uintptr_t currentState=0;uint8_t snapshot=0,flag=0;
};
template<class Ops>inline void shaderSystemNormalDrawCandidate(
 ShaderSystemNormalDrawStateCandidate&state,uintptr_t shader,uintptr_t parameters,
 uintptr_t shaderState,uint32_t compression,uint32_t decoded,uint32_t alpha,Ops&ops){
 ops.resetApi();
 const uint32_t index=ops.select(shader,parameters)|alpha;
 if(!ops.available(shaderState,index))return;
 const uint32_t cachedFlags=ops.parameterFlags(parameters);
 ops.beginApi();if(cachedFlags&0x1000u)ops.api100(0);
 ops.pushLogging(1,1);ops.registerListener();ops.setResponsePolicy();
 state.currentState=shaderState;state.snapshot=uint8_t(index);state.flag=0;
 ops.useSnapshot(uint16_t(ops.snapshotWord(state.currentState,state.snapshot)));
 // useSnapshot may replace the state, index, array or API. Reload live fields.
 const auto cell=ops.contextCell(state.currentState,state.snapshot);
 if(ops.context(cell)&&ops.contextKey(ops.context(cell))!=decoded){
  ops.dirtyContext(ops.context(cell));ops.setContextKey(ops.context(cell),decoded);
 }
 const auto tail=ops.snapshotTail(state.currentState,state.snapshot);
 ops.draw(shader,parameters,index,compression,cell,tail);
 ops.popLogging(1);ops.clearDiagnosticStorage();state.currentState=0;
 ops.resetMaterialMode(0);
}
