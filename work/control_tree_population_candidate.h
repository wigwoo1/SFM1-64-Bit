#pragma once
#include "control_group_selection_state_candidate.h"
//100907d0. Adapter supplies live references, original IsA, inherited visibility,
// model record allocation and settings. It must prepend created row nodes.
// Original assumes finite acyclic hierarchy and successful control allocation.
template<class Adapter>
void populateControlTreeCandidate(ControlGroupSelectionNodeCandidate* node,Adapter& a){
 if(!node)return;
 auto* group=a.groupForNode(*node);
 if(!group)return;
 const auto prior=a.recordCount();
 for(auto i=a.controlCount(group);i>0;){
  --i;
  auto* control=a.controlAt(group,i);
  if(control&&a.isControl(control))a.addControl(control,node);
 }
 for(auto i=a.childCount(group);i>0;){
  --i;
  auto* child=a.childAt(group,i);
  if(!child||!a.isGroup(child))continue;
  if(!a.visible(child)&&!a.showHiddenGroups())continue;
  if(a.controlCount(child)==0&&a.childCount(child)==0)continue;
  auto* childNode=a.addGroup(child,node);
  populateControlTreeCandidate(childNode,a);
 }
 if(node->aggregate==0&&a.recordCount()>prior)node->aggregate=1;
}
