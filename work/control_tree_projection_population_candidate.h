#pragma once
#include <deque>
#include <limits>
#include <utility>
#include <unordered_map>
#include "control_record_lookup_candidate.h"
#include "animation_set_projection_group_candidate.h"
#include "control_tree_projection_references_candidate.h"
#include "control_tree_population_candidate.h"
#include "control_tree_model_navigation_candidate.h"
#include "control_selection_background_candidate.h"
#include "control_model_foreground_candidate.h"
// Owns stable row/record storage. Original IsA and visibility supplied by bridge.
template<class Projection,class Semantics>
struct ControlTreeProjectionPopulationCandidate {
 using Element=typename std::remove_pointer<decltype(std::declval<Projection&>().scene.resolve(0u))>::type;
 using Record=ControlRecordCandidate;
 struct Leaves{ControlGroupSelectionNodeCandidate nodes[9];};
 Projection& projection;Semantics& semantics;
 std::deque<Record> records;
 std::deque<Leaves> leaves;
 std::deque<ControlGroupSelectionNodeCandidate> groups;
 std::unordered_map<const std::uint32_t*,Record*> recordBindings;
 std::unordered_map<std::uint32_t,std::size_t> elementIndices;
 ControlTreeProjectionPopulationCandidate(Projection& p,Semantics& s):projection(p),semantics(s){
  elementIndices.reserve(p.elementHandles.size());
  for(std::size_t i=0;i<p.elementHandles.size();++i)elementIndices.emplace(p.elementHandles[i],i);
 }
 ControlTreeProjectionPopulationCandidate(const ControlTreeProjectionPopulationCandidate&)=delete;
 ControlTreeProjectionPopulationCandidate& operator=(const ControlTreeProjectionPopulationCandidate&)=delete;
 Record* recordForNode(const ControlGroupSelectionNodeCandidate& n){
  auto found=recordBindings.find(n.controlSelectionBits);
  return found==recordBindings.end()?nullptr:found->second;
 }
 Element* elementForNode(const ControlGroupSelectionNodeCandidate& n,
  std::uint32_t* available=nullptr,std::uint32_t* selected=nullptr){
  if(n.kind<4)return projection.scene.resolve(n.elementHandle);
  return lookupControlRecordCandidate<Element>(n.kind,recordForNode(n),available,selected,
   knownControlLeafMaskCandidate,[&](std::uint32_t h){return projection.scene.resolve(h);});
 }
 // Base labels only. Original display wrapper's metadata decorations are pending.
 const char* baseLabelForNode(const ControlGroupSelectionNodeCandidate& node){
  auto* element=elementForNode(node);
  if(!element)return controlNodeBaseLabelCandidate(node.kind,false,nullptr);
  if(node.kind>4)return controlNodeBaseLabelCandidate(node.kind,true,nullptr);
  auto found=elementIndices.find(element->record.handle);
  if(found==elementIndices.end())throw std::runtime_error("unbound control-tree element name");
  const auto name=projection.graph.elements.at(found->second).name;
  return controlNodeBaseLabelCandidate(node.kind,true,name==0xffffffffu?nullptr:
   projection.graph.strings.at(name).c_str());
 }
 std::uint32_t flagsForNode(const ControlGroupSelectionNodeCandidate* node,
  std::int32_t row,std::int32_t column,bool modelIdentityMatches){
  return controlModelItemFlagsCandidate(
   controlModelBaseFlagsCandidate(row,column,modelIdentityMatches),node!=nullptr,
   column,node?node->kind:0,
   [&](){return groupForNode(*node)!=nullptr;},
   [&](){auto* e=elementForNode(*node);return e&&semantics.isTransform(e);});
 }
 ControlTreeIndexCandidate index(ControlGroupSelectionNodeCandidate* root,
  std::int32_t row,std::int32_t column,const ControlTreeIndexCandidate& parent={}){
  return controlTreeIndexCandidate(root,this,row,column,parent);
 }
 ControlTreeIndexCandidate parent(ControlGroupSelectionNodeCandidate* root,
  const ControlTreeIndexCandidate& child){return controlTreeParentCandidate(root,this,child);}
 std::uint32_t rowCount(ControlGroupSelectionNodeCandidate* root,
  const ControlTreeIndexCandidate& parent={}){return controlTreeRowCountCandidate(root,parent);}
 std::int32_t columnCount()const{return controlTreeColumnCountCandidate();}
 std::uint32_t flags(const ControlTreeIndexCandidate& index){
  return flagsForNode(index.node,index.row,index.column,index.model==this);
 }
 std::uint32_t selectionState(const ControlGroupSelectionNodeCandidate& node)const{
  return readControlSelectionCandidate(node,knownControlLeafMaskCandidate);
 }
 std::optional<ControlSelectionBackgroundCandidate> selectionBackground(
  const ControlGroupSelectionNodeCandidate& node)const{
  return controlSelectionBackgroundCandidate(selectionState(node));
 }
 std::uint32_t foreground(const ControlGroupSelectionNodeCandidate& node){
  return controlModelForegroundCandidate(node.kind,node.inheritedFlag!=0,
   [&](){return groupForNode(node);},
   [&](Element* e){return semantics.groupColor(e);},
   [&](){return elementForNode(node);},
   [&](Element* e){return semantics.controlColorOwner(e);},
   [&](Element* e){return semantics.controlColor(e);});
 }
 std::uint32_t refreshSelectionState(ControlGroupSelectionNodeCandidate* root){
  return updateControlGroupSelectionCandidate(root,knownControlLeafMaskCandidate);
 }
 static std::int32_t count(std::size_t n){
  if(n>static_cast<std::size_t>((std::numeric_limits<std::int32_t>::max)()))throw std::overflow_error("control tree count");
  return static_cast<std::int32_t>(n);
 }
 std::int32_t recordCount(){return count(records.size());}
 Element* groupForNode(const ControlGroupSelectionNodeCandidate& n){
  return animationSetProjectionGroupCandidate(projection,n.kind,n.elementHandle,
   [&](Element* e){return semantics.isGroup(e);},[&](Element* e){return semantics.isAnimationSet(e);});
 }
 std::int32_t controlCount(Element* e){return count(controlTreeProjectionReferenceCountCandidate(projection,e,"controls"));}
 std::int32_t childCount(Element* e){return count(controlTreeProjectionReferenceCountCandidate(projection,e,"children"));}
 Element* controlAt(Element* e,std::int32_t i){return controlTreeProjectionReferenceAtCandidate(projection,e,"controls",i);}
 Element* childAt(Element* e,std::int32_t i){return controlTreeProjectionReferenceAtCandidate(projection,e,"children",i);}
 // Original controls-array gate is base CDmElement, not a control subclass.
 bool isControl(Element* e){return semantics.isElement(e);}
 bool isGroup(Element* e){return semantics.isGroup(e);}
 bool visible(Element* e){return semantics.visible(e);}
 bool showHiddenGroups(){return semantics.showHiddenGroups();}
 void addControl(Element* e,ControlGroupSelectionNodeCandidate* parent){
  const bool transform=semantics.isTransform(e);
  records.push_back({e->record.handle,transform?0x7eu:0x80u,0});
  recordBindings.emplace(&records.back().selectionBits,&records.back());
  leaves.emplace_back();auto& block=leaves.back();
  if(transform)initializeTransformControlLeavesCandidate(block.nodes,&records.back().selectionBits,parent);
  else initializeControlLeafCandidate(block.nodes[0],4,&records.back().selectionBits,parent);
 }
 ControlGroupSelectionNodeCandidate* addGroup(Element* e,ControlGroupSelectionNodeCandidate* parent){
  groups.emplace_back();initializeControlGroupNodeCandidate(groups.back(),e->record.handle,parent);
  return &groups.back();
 }
 // Caller owns root; rebuild into a fresh adapter and root after edits.
 void populate(ControlGroupSelectionNodeCandidate& root){populateControlTreeCandidate(&root,*this);}
};
