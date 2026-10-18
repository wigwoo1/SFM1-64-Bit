#pragma once
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "session_ik_runtime.h"
#include "float_components_undo_replay.h"
#include <variant>
#include <type_traits>
using RecoveredOwnedIkUndoRecordCandidate=std::variant<RecoveredFloatComponentsUndoPayloadCandidate<3>,RecoveredFloatComponentsUndoPayloadCandidate<4>>;
struct RecoveredOwnedSessionIkPassResult {size_t executed=0,skipped=0;SessionAttributeChanges changes;std::vector<RecoveredOwnedIkUndoRecordCandidate> undoRecords;};
// Explicit isolated IK pass in serialized element order, not native scheduling.
// Selected controls are supplied by caller; logical typed flags/owner dirty
// state are bound, native class defaults/undo/derived notifications pending.
template<class Projection,class SelectControls>
RecoveredOwnedSessionIkPassResult executeOwnedSessionIkPassCandidate(Projection&session,SelectControls selectControls,int phase=0,bool recordUndo=false){
 RecoveredOwnedSessionIkPassResult result;GraphTransformHierarchy hierarchy(session.graph);
 if(recordUndo){size_t count=0;for(const auto&e:session.graph.elements)if(session.graph.strings.at(e.type)=="DmeRigIKConstraintOperator")++count;result.undoRecords.reserve(count*5);}
 for(uint32_t i=0;i<session.graph.elements.size();++i)if(session.graph.strings.at(session.graph.elements[i].type)=="DmeRigIKConstraintOperator"){
 auto controls=selectControls(i);std::array<std::array<float,3>,3>values{};std::array<const std::array<float,3>*,3>defaults{};for(unsigned j=0;j<3;++j)if(controls[j]!=0xffffffffu){values[j]=graphSelectedControlDefaultPositionCandidate(session.graph,controls[j],{0,0,0});defaults[j]=&values[j];}
 auto write=[&](uint32_t target,const char*name,const auto&value){if(recordUndo){auto captured=session.captureFloatUndo(target,name,value);if(session.setFloatComponents(target,name,value,phase)){result.undoRecords.emplace_back(std::move(captured));result.changes.attributes.emplace_back(target,name);}}else if(session.setFloatComponents(target,name,value,phase))result.changes.attributes.emplace_back(target,name);};
 if(executeSessionIkWithWritersCandidate(session.graph,hierarchy,i,{0,0,0},defaults,[&](uint32_t t,const auto&q){write(t,"orientation",q);},[&](uint32_t t,const auto&v){write(t,"position",v);}))++result.executed;else ++result.skipped;
 }
 return result;
}
// Isolated pass history, not native transaction/stack/grouping reconstruction.
template<class Projection>size_t replayOwnedIkUndoRecordsCandidate(Projection&session,const std::vector<RecoveredOwnedIkUndoRecordCandidate>&records,bool reverse,int phase=0){size_t dispatched=0;auto replay=[&](const auto&record){if(std::visit([&](const auto&typed){return session.replayFloatUndo(typed,reverse,phase);},record))++dispatched;};if(reverse){for(auto it=records.rbegin();it!=records.rend();++it)replay(*it);}else for(const auto&record:records)replay(record);return dispatched;}
