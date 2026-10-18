#include "graph_editor_keymode_state_candidate.h"
#include "timeline_head_lock_state_candidate.h"
#include "control_group_selection_state_candidate.h"
#include "control_model_lock_toggle_candidate.h"
#include "control_lock_affine_candidate.h"
#include "control_lock_parent_guard_candidate.h"
#include "control_record_lookup_candidate.h"
#include "channel_assignment_gate_candidate.h"
#include "incoming_reference_list_candidate.h"
#include "attribute_token_release_candidate.h"
#include "control_model_item_flags_candidate.h"
#include "control_model_group_lookup_candidate.h"
#include "control_group_defaults_candidate.h"
#include "control_group_name_comparator_candidate.h"
#include "children_reference_cleanup_candidate.h"
#include "control_handle_array_insert_candidate.h"
#include "animation_set_control_model_adapter_candidate.h"
struct CompileOnlyControlGraph {
 void* resolve(std::uint32_t){return nullptr;}
 bool isGroup(void*){return false;}
 bool isAnimationSet(void*){return false;}
 void* rootGroup(void*){return nullptr;}
 bool isTransformControl(const ControlGroupSelectionNodeCandidate&){return false;}
};
std::uint32_t compileAnimationSetControlFlags(std::uint32_t base,
 const ControlGroupSelectionNodeCandidate* node,std::int32_t column) {
 CompileOnlyControlGraph graph;
 return animationSetControlFlagsCandidate<void>(base,node,column,graph);
}
std::int32_t compileControlHandleInsert(ControlHandleArrayCandidate& a,std::int32_t index) {
 return insertControlHandleCandidate(a,index,[](ControlHandleArrayCandidate&,std::int32_t){},
 [](std::uint32_t*,std::uint32_t*,std::int32_t){},[]{return 0xffffffffu;});
}
void compileChildrenReferenceCleanup(std::int32_t first,std::int32_t last,bool flag) {
 childrenReferenceCleanupCandidate(first,last,flag,[]{return false;},
 [](std::int32_t,std::int32_t){},[](std::int32_t){return 0xffffffffu;},[](std::uint32_t){});
}
int compileControlGroupNameComparison(const char* first,const char* second) {
 return controlGroupNameComparatorCandidate(first,second);
}
int compileControlGroupSortNameComparison(const char* first,const char* second,bool forward) {
 return controlGroupSortNameComparatorCandidate(first,second,forward);
}
ControlGroupDefaultsCandidate compileControlGroupDefaults() {return {};}
void* compileControlGroupLookup(std::int32_t kind,std::uint32_t handle) {
 return controlModelGroupLookupCandidate<void>(kind,handle,
 [](std::uint32_t)->void*{return nullptr;},[](void*){return false;},
 [](void*){return false;},[](void*)->void*{return nullptr;});
}
std::uint32_t compileControlModelFlags(std::uint32_t base,bool present,
 std::int32_t column,std::int32_t kind,bool group,bool transform) {
 return controlModelItemFlagsCandidate(base,present,column,kind,[group]{return group;},[transform]{return transform;});
}
void compileAttributeTokenRelease(std::uint32_t token, AttributeTokenSlotCandidate* slots,
 std::uint32_t count, std::uint32_t& liveCount) {
 releaseAttributeTokenSlotCandidate(token,slots,count,liveCount,[](std::uint32_t){});
}
void compileIncomingReferenceList(IncomingReferenceListCandidate& list,
 IncomingReferenceLinkCandidate* allocation, std::uint32_t token) {
 addIncomingReferenceCandidate(list,[allocation]{return allocation;},[token]{return token;});
 removeIncomingReferenceCandidate(list,[token]{return token;},[](IncomingReferenceLinkCandidate*){});
}
bool compileChannelAssignmentGate(std::uint16_t& flags, std::uint32_t oldHandle,
 std::uint32_t newHandle, std::uint32_t expectedType) {
 return channelAssignmentGateCandidate(flags,oldHandle,newHandle,expectedType,
 [](std::uint32_t)->void* {return nullptr;},
 [](void*,std::uint32_t){return false;},
 []{return 0;},[]{},[]{});
}
void* compileControlRecordLookup(int32_t kind,const ControlRecordCandidate* record,
 uint32_t* available,uint32_t* selected){
 return lookupControlRecordCandidate<void>(kind,record,available,selected,
 knownControlLeafMaskCandidate,[](uint32_t)->void*{return nullptr;});
}
const char* compileControlNodeLabel(int32_t kind,bool resolved,const char* name){
 return controlNodeBaseLabelCandidate(kind,resolved,name);
}
int32_t compileControlParentGuard(const ControlGroupSelectionNodeCandidate* receiver,
 const ControlGroupSelectionNodeCandidate* candidate){
 return controlLockParentGuardCandidate(receiver,candidate,
 [](const ControlGroupSelectionNodeCandidate* node)->const ControlGroupSelectionNodeCandidate*{return node->parent;},
 [](const ControlGroupSelectionNodeCandidate*)->const ControlGroupSelectionNodeCandidate*{return nullptr;});
}
void compileControlLockAffine(const float* a,const float* b,float* out){
 concatenateControlLockAffineCandidate(a,b,out);
}
bool compileControlLockToggle(int32_t kind,ControlModelLockValuesCandidate& values){
 return toggleControlModelLockValuesCandidate(kind,values);
}
std::optional<int32_t> compileControlLockCheckState(int32_t kind,bool resolved,
 const ControlModelLockValuesCandidate& values){
 return controlModelLockCheckStateCandidate(kind,resolved,values);
}
ControlGroupSelectionNodeCandidate* compileControlParentSelection(bool byte64,bool enabled,
 ControlGroupSelectionNodeCandidate* overrideParent,ControlGroupSelectionNodeCandidate* fallback){
 return selectControlLockParentCandidate(byte64,enabled,overrideParent,[fallback]{return fallback;});
}
uint32_t compileControlSelection(ControlGroupSelectionNodeCandidate* node){
 return updateControlGroupSelectionCandidate(node,knownControlLeafMaskCandidate);
}
void compileTransformControlConstruction(ControlGroupSelectionNodeCandidate (&nodes)[9],
 const uint32_t* bits,ControlGroupSelectionNodeCandidate* parent){
 initializeTransformControlLeavesCandidate(nodes,bits,parent);
}
int32_t compileControlModelRow(const ControlGroupSelectionNodeCandidate& parent,
 const ControlGroupSelectionNodeCandidate* child){
 return controlChildCountCandidate(parent)?controlChildRowCandidate(parent,child):-1;
}
