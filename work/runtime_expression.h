#pragma once
#include "session_runtime_handles.h"
inline bool executeRuntimeExpression(Graph&graph,const SessionRuntimeHandles&runtime,uint32_t handle,SessionAttributeChanges*changes=nullptr){
 auto record=reinterpret_cast<const SessionElementRecord*>(runtime.table.resolve(handle));
 if(!record)return false;return executeSessionExpression(graph,record->graphIndex,changes);
}
