#pragma once
#include "control_group_selection_state_candidate.h"
// Portable index, not Qt ABI. Original navigation checks model nonnull,
// unlike base flags which check model identity. Parent preserves column.
struct ControlTreeIndexCandidate {
 std::int32_t row=-1,column=-1;
 ControlGroupSelectionNodeCandidate* node=nullptr;
 const void* model=nullptr;
 bool valid()const{return row>=0&&column>=0&&model!=nullptr;}
};
inline ControlTreeIndexCandidate controlTreeIndexCandidate(
 ControlGroupSelectionNodeCandidate* root,const void* model,std::int32_t row,
 std::int32_t column,const ControlTreeIndexCandidate& parent={}){
 auto* owner=parent.valid()?parent.node:root;
 if(!owner)return {};
 auto* child=controlChildAtCandidate(*owner,row);
 if(!child)return {};
 return {row,column,child,model};
}
inline ControlTreeIndexCandidate controlTreeParentCandidate(
 ControlGroupSelectionNodeCandidate* root,const void* model,
 const ControlTreeIndexCandidate& index){
 if(!index.valid()||!index.node||!index.node->parent)return {};
 auto* parent=index.node->parent;
 if(parent==root||!parent->parent)return {};
 const auto row=controlChildRowCandidate(*parent->parent,parent);
 if(row<0)return {};
 return {row,index.column,parent,model};
}
inline std::uint32_t controlTreeRowCountCandidate(
 ControlGroupSelectionNodeCandidate* root,const ControlTreeIndexCandidate& parent={}){
 auto* owner=parent.valid()?parent.node:root;
 return owner?controlChildCountCandidate(*owner):0u;
}
inline std::int32_t controlTreeColumnCountCandidate(){return 4;}
