#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "reference_owned_dmx_projection.h"
#include "animation_set_projection_group_candidate.h"
#include "control_tree_projection_references_candidate.h"
#include "control_tree_projection_population_candidate.h"
#include "control_model_display_decoration_candidate.h"
#include "control_tree_projection_color_candidate.h"
#include "control_tree_projection_bool_candidate.h"
#include "control_tree_projection_incoming_owner_candidate.h"
#include "control_tree_projection_visibility_candidate.h"
#include <iostream>
#include <functional>
#include <unordered_set>
struct SessionTreeAuditSemantics {
 RecoveredOwnedDmxReferenceProjectionCandidate<Graph>& p;
 std::unordered_map<uint32_t,uint32_t>& indices;
 bool type(RecoveredOwnedSceneElementCandidate* e,const char* name){
  return p.graph.strings.at(p.graph.elements.at(indices.at(e->record.handle)).type)==name;
 }
 bool isGroup(RecoveredOwnedSceneElementCandidate* e){return type(e,"DmeControlGroup");}
 bool isAnimationSet(RecoveredOwnedSceneElementCandidate* e){return type(e,"DmeAnimationSet");}
 bool isTransform(RecoveredOwnedSceneElementCandidate* e){return type(e,"DmeTransformControl");}
 bool isElement(RecoveredOwnedSceneElementCandidate* e){return e!=nullptr;}
 bool visible(RecoveredOwnedSceneElementCandidate*){return true;}
 bool showHiddenGroups(){return true;}
};
int main(int argc,char** argv){try{
 if(argc==2){
  auto source=loadGraph(argv[1]);
  RecoveredOwnedDmxReferenceProjectionCandidate<Graph> p(source,[](const std::string&,auto&)->uint32_t{throw std::runtime_error("external reference unsupported in this audit");});
  std::unordered_map<uint32_t,uint32_t> indices;
  for(uint32_t i=0;i<p.elementHandles.size();++i) indices.emplace(p.elementHandles[i],i);
  auto type=[&](auto* e,const char* name){return p.graph.strings.at(p.graph.elements.at(indices.at(e->record.handle)).type)==name;};
  auto group=[&](auto*e){return type(e,"DmeControlGroup");};
  auto animation=[&](auto*e){return type(e,"DmeAnimationSet");};
  size_t sets=0,groups=0,flagChecks=0,arrayReferences=0;
  size_t treeRecords=0,treeGroups=0;
  size_t treeLabels=0;
  size_t colorValues=0;
  size_t boolValues=0;
  size_t treeIndexChecks=0;
  SessionTreeAuditSemantics treeSemantics{p,indices};
  auto wireArray=[&](uint32_t i,const char* name)->const Attribute*{
   for(const auto& a:source.elements.at(i).attributes)
    if(a.type==15&&source.strings.at(a.name)==name)return &a;
   return nullptr;
  };
  std::vector<uint32_t> expectedRecords;
  std::unordered_set<uint32_t> activeGroups;
  std::function<void(uint32_t)> collectWire=[&](uint32_t i){
   if(!activeGroups.insert(i).second)throw std::runtime_error("cyclic group fixture");
   const auto* controls=wireArray(i,"controls");
   if(controls)for(auto it=controls->values.rbegin();it!=controls->values.rend();++it)
    if(it->reference!=0xffffffffu)expectedRecords.push_back(p.elementHandles.at(it->reference));
   const auto* children=wireArray(i,"children");
   if(children)for(auto it=children->values.rbegin();it!=children->values.rend();++it){
    auto ref=it->reference;if(ref==0xffffffffu)continue;
    if(source.strings.at(source.elements.at(ref).type)!="DmeControlGroup")continue;
    const auto* c=wireArray(ref,"controls");const auto* g=wireArray(ref,"children");
    if((!c||c->values.empty())&&(!g||g->values.empty()))continue;
    collectWire(ref);
   }
   activeGroups.erase(i);
  };
  auto verifyFlags=[&](int kind,uint32_t handle,bool hasGroup){
   const uint32_t expected[4]={0x21u,0u,0x30u,hasGroup?0x30u:0x20u};
   for(int column=0;column<4;++column){
    const auto actual=animationSetProjectionIndexFlagsCandidate(p,0,column,true,true,kind,handle,
       group,animation,[]()->bool{throw std::runtime_error("unexpected transform lookup for group/set node");});
    const auto wanted=column==2&&kind==3?0x20u:expected[column];
    if(actual!=wanted)throw std::runtime_error("session-backed UI flags mismatch");
    ++flagChecks;
   }
  };
  for(uint32_t i=0;i<p.elementHandles.size();++i){
   auto* owner=p.scene.resolve(p.elementHandles[i]);
   if(group(owner)){
    for(const auto& attr:source.elements[i].attributes){
     const auto& name=source.strings.at(attr.name);
     if(attr.type==4&&(name=="visible"||name=="selectable"||name=="snappable")){
      auto value=controlTreeProjectionBoolCandidate(p,i,name.c_str());
      if(!value||*value!=static_cast<uint8_t>(attr.values.at(0).wire.at(0)))throw std::runtime_error("group bool mismatch");
      ++boolValues;
     }
     if(attr.type==8&&(name=="groupColor"||name=="controlColor")){
      const auto value=controlTreeProjectionColorCandidate(p,i,name.c_str());
      if(!value||*value!=wireU32(attr.values.at(0)))throw std::runtime_error("group color value mismatch");
      ++colorValues;
     }
     if(attr.type!=15||(name!="children"&&name!="controls"))continue;
     auto* live=controlTreeProjectionAttributeCandidate(p,owner,name.c_str(),15);
     if(!live||live->references().size()!=attr.values.size())throw std::runtime_error("tree array count mismatch");
     for(size_t j=0;j<attr.values.size();++j){
      auto* expected=attr.values[j].reference==0xffffffffu?nullptr:p.scene.resolve(p.elementHandles.at(attr.values[j].reference));
      if(controlTreeProjectionReferenceAtCandidate(p,owner,name.c_str(),static_cast<int32_t>(j))!=expected)throw std::runtime_error("tree array target mismatch");
      ++arrayReferences;
     }
    }
    if(animationSetProjectionGroupCandidate(p,3,p.elementHandles[i],group,animation)!=owner) throw std::runtime_error("direct group mismatch");
    verifyFlags(3,p.elementHandles[i],true);
    ++groups;
   }
   if(!animation(owner)) continue;
   decltype(owner) expected=nullptr;
   for(const auto& attr:source.elements[i].attributes) if(source.strings.at(attr.name)=="rootControlGroup" && attr.type==1 && attr.values.size()==1){
    const auto ref=attr.values[0].reference;
    if(ref!=0xffffffffu) expected=p.scene.resolve(p.elementHandles.at(ref));
    break;
   }
   if(animationSetProjectionGroupCandidate(p,2,p.elementHandles[i],group,animation)!=expected) throw std::runtime_error("session group mismatch");
   expectedRecords.clear();activeGroups.clear();
   if(expected)collectWire(indices.at(expected->record.handle));
   ControlTreeProjectionPopulationCandidate<decltype(p),SessionTreeAuditSemantics> tree(p,treeSemantics);
   ControlGroupSelectionNodeCandidate rootNode{};rootNode.kind=2;rootNode.elementHandle=p.elementHandles[i];
   tree.populate(rootNode);
   if(tree.records.size()!=expectedRecords.size())throw std::runtime_error("session tree record count mismatch");
   for(size_t j=0;j<expectedRecords.size();++j){
    const auto& record=tree.records[j];
    if(record.elementHandle!=expectedRecords[j])throw std::runtime_error("session tree record order mismatch");
    auto* e=p.scene.resolve(record.elementHandle);
    if(record.availabilityBits!=(treeSemantics.isTransform(e)?0x7eu:0x80u))throw std::runtime_error("session tree record mask mismatch");
   }
   treeRecords+=tree.records.size();treeGroups+=tree.groups.size();
   for(const auto& block:tree.leaves){
    if(tree.selectionState(block.nodes[0])!=1)throw std::runtime_error("initial session selection mismatch");
   }
   if(tree.refreshSelectionState(&rootNode)!=(tree.records.empty()?0u:1u))throw std::runtime_error("initial group selection mismatch");
   std::function<void(ControlGroupSelectionNodeCandidate*)> checkLabels=[&](auto* node){
    auto* e=tree.elementForNode(*node);
    if(!e)throw std::runtime_error("unexpected missing fixture tree element");
    static const char* axisLabels[]={"pos","x","y","z","rot","x","y","z"};
    const char* wanted=node->kind>4?axisLabels[node->kind-5]:
     source.strings.at(source.elements.at(indices.at(e->record.handle)).name).c_str();
    if(std::string(tree.baseLabelForNode(*node))!=wanted)throw std::runtime_error("session tree label mismatch");
    ++treeLabels;
    for(auto* child=node->firstChild;child;child=child->nextSibling)checkLabels(child);
   };
   checkLabels(&rootNode);
   std::function<void(ControlGroupSelectionNodeCandidate*,ControlTreeIndexCandidate)> checkModel=
    [&](auto* node,auto parentIndex){
     int32_t row=0;
     for(auto* child=node->firstChild;child;child=child->nextSibling,++row){
      for(int32_t column=0;column<4;++column){
       const auto childIndex=tree.index(&rootNode,row,column,parentIndex);
       if(!childIndex.valid()||childIndex.node!=child||childIndex.model!=&tree)throw std::runtime_error("session tree index mismatch");
       const auto actualParent=tree.parent(&rootNode,childIndex);
       if(node==&rootNode){if(actualParent.valid())throw std::runtime_error("root parent must be invalid");}
       else if(actualParent.node!=node||actualParent.column!=column||actualParent.row!=parentIndex.row)
        throw std::runtime_error("session parent index mismatch");
       if(tree.flags(childIndex)!=tree.flagsForNode(child,row,column,true))throw std::runtime_error("index flags bridge mismatch");
       ++treeIndexChecks;
      }
      checkModel(child,tree.index(&rootNode,row,0,parentIndex));
     }
     if(tree.rowCount(&rootNode,parentIndex)!=static_cast<uint32_t>(row)||tree.columnCount()!=4)
      throw std::runtime_error("model dimensions mismatch");
     if(tree.index(&rootNode,row,0,parentIndex).valid())throw std::runtime_error("out of range row accepted");
    };
   checkModel(&rootNode,{});
   verifyFlags(2,p.elementHandles[i],expected!=nullptr);
   ++sets;
  }
  if(!sets||!groups) throw std::runtime_error("fixture lacks animation sets or groups");
  std::cout<<"session_elements="<<source.elements.size()<<" animation_sets_checked="<<sets<<" control_groups_checked="<<groups<<" ui_flag_checks="<<flagChecks<<" exact_type_fixture_only=1 original_runtime_parity=unverified\n";
  std::cout<<"tree_array_references_checked="<<arrayReferences<<'\n';
  std::cout<<"tree_base_labels_checked="<<treeLabels<<'\n';
  std::cout<<"group_color_values_checked="<<colorValues<<'\n';
  std::cout<<"group_bool_values_checked="<<boolValues<<'\n';
  std::cout<<"tree_model_index_checks="<<treeIndexChecks<<'\n';
  std::cout<<"populated_tree_records="<<treeRecords<<" populated_group_rows="<<treeGroups<<" visibility_fixture=all_visible exact_types_only=1 original_runtime_parity=unverified\n";
  return 0;
 }
 static_assert(sizeof(void*)==8,"x64 required");
 Graph source;source.strings={"DmeAnimationSet","DmeControlGroup","set","groupA","groupB","rootControlGroup","children","controls","groupColor","controlColor"};
 for(unsigned i=0;i<3;++i){Element e{};e.type=i?1:0;e.name=i+2;e.id[0]=static_cast<char>(i+1);source.elements.push_back(e);}
 Attribute root{};root.name=5;root.type=1;Value target;target.reference=1;root.values.push_back(target);source.elements[0].attributes.push_back(root);
 Attribute children{};children.name=6;children.type=15;Value child;child.reference=2;children.values.push_back(child);source.elements[1].attributes.push_back(children);
 Attribute controls{};controls.name=7;controls.type=15;Value unset;controls.values.push_back(unset);source.elements[1].attributes.push_back(controls);
 Attribute groupColor{};groupColor.name=8;groupColor.type=8;Value groupColorValue;groupColorValue.wire={0x22,0x33,0x44,0x55};groupColor.values.push_back(groupColorValue);source.elements[1].attributes.push_back(groupColor);
 Attribute controlColor{};controlColor.name=9;controlColor.type=8;Value controlColorValue;controlColorValue.wire={0x55,0x66,0x77,static_cast<char>(0x88)};controlColor.values.push_back(controlColorValue);source.elements[1].attributes.push_back(controlColor);
 RecoveredOwnedDmxReferenceProjectionCandidate<Graph> p(source,[](const std::string&,auto&)->uint32_t{throw std::runtime_error("unexpected external reference");});
 auto* set=p.scene.resolve(p.elementHandles[0]);auto* a=p.scene.resolve(p.elementHandles[1]);auto* b=p.scene.resolve(p.elementHandles[2]);
 auto group=[&](auto*e){return e==a||e==b;};auto animation=[&](auto*e){return e==set;};
 auto lookup=[&](int kind,uint32_t h){return animationSetProjectionGroupCandidate(p,kind,h,group,animation);};
 auto check=[](bool ok){if(!ok)throw std::runtime_error("group lookup assertion");};
 p.graph.strings.push_back("visible");
 Attribute visibility{};visibility.name=static_cast<uint32_t>(p.graph.strings.size()-1);visibility.type=4;
 Value visibleValue;visibleValue.wire={2};visibility.values.push_back(visibleValue);
 p.graph.elements[1].attributes.push_back(visibility);
 check(controlTreeProjectionBoolCandidate(p,1,"visible").value()==2);
 p.graph.elements[1].attributes.back().values[0].wire[0]=0;
 check(controlTreeProjectionBoolCandidate(p,1,"visible").value()==0);
 check(!controlTreeProjectionBoolCandidate(p,2,"visible"));
 check(!controlTreeProjectionBoolCandidate(p,1,"groupColor"));
 p.graph.elements[1].attributes.pop_back();p.graph.strings.pop_back();
 auto childAt=[&](int i){return controlTreeProjectionReferenceAtCandidate(p,a,"children",i);};
 check(controlTreeProjectionReferenceCountCandidate(p,a,"children")==1&&childAt(0)==b);
 check(!childAt(-1)&&!childAt(1));
 check(controlTreeProjectionReferenceCountCandidate(p,a,"controls")==1);
 check(controlTreeProjectionReferenceCountCandidate(p,b,"controls")==0);
 check(!controlTreeProjectionReferenceAtCandidate(p,a,"controls",0));
 check(!controlTreeProjectionAttributeCandidate(p,a,"children",1));
 check(!controlTreeProjectionAttributeCandidate(p,a,"missing",15));
 auto* childAttribute=controlTreeProjectionAttributeCandidate(p,a,"children",15);
 check(p.scene.setArrayReferenceRange(childAttribute->token,0,{p.elementHandles[0]},0,0xffffffffu,[](uint32_t,uint32_t){return true;}));
 check(childAt(0)==set&&p.graph.elements[1].attributes[0].values[0].reference==2);
 check(p.scene.setArrayReferenceRange(childAttribute->token,0,{0xffffffffu},0,0xffffffffu,[](uint32_t,uint32_t){return true;}));
 check(!childAt(0)&&controlTreeProjectionReferenceCountCandidate(p,a,"children")==1);
 check(p.scene.setArrayReferenceRange(childAttribute->token,0,{p.elementHandles[2]},0,0xffffffffu,[](uint32_t,uint32_t){return true;}));
 check(childAt(0)==b);
 struct TreeSemantics{
  decltype(a) groupA,groupB,control;
  decltype(p)& projection;
  bool isGroup(decltype(a) e){return e==groupA||e==groupB;}
  bool isAnimationSet(decltype(a) e){return e==control;}
  bool isElement(decltype(a) e){return e!=nullptr;}
  bool isTransform(decltype(a) e){return e==control;}
  bool visible(decltype(a)){return true;}
  bool showHiddenGroups(){return false;}
  uint32_t groupColor(decltype(a) e){return controlTreeProjectionColorCandidate(projection,e==groupA?1u:2u,"groupColor").value();}
  uint32_t controlColor(decltype(a) e){return controlTreeProjectionColorCandidate(projection,e==groupA?1u:2u,"controlColor").value();}
  decltype(a) controlColorOwner(decltype(a) e){return e==control?groupA:nullptr;}
 } semantics{a,b,set,p};
 check(!controlTreeProjectionColorCandidate(p,2,"groupColor"));
 check(!controlTreeProjectionColorCandidate(p,1,"children"));
 auto* controlAttribute=controlTreeProjectionAttributeCandidate(p,a,"controls",15);
 // Range writer replaces existing entries; structural insertion is separate.
 check(p.scene.setArrayReferenceRange(controlAttribute->token,0,{p.elementHandles[0]},0,0xffffffffu,[](uint32_t,uint32_t){return true;}));
 ControlTreeProjectionPopulationCandidate<decltype(p),TreeSemantics> tree(p,semantics);
 ControlGroupSelectionNodeCandidate treeRoot{};treeRoot.kind=2;treeRoot.elementHandle=set->record.handle;
 tree.populate(treeRoot);
 check(tree.recordCount()==1&&tree.records.front().elementHandle==set->record.handle&&tree.records.front().availabilityBits==0x7e);
 check(treeRoot.aggregate==1&&controlChildCountCandidate(treeRoot)==1);
 check(controlChildCountCandidate(*treeRoot.firstChild)==2);
 auto incomingOwner=[&](decltype(a) target,const char* name){
  return controlTreeProjectionIncomingOwnerCandidate(p,target,name,group,[](auto*){return 10u;});
 };
 check(incomingOwner(set,"controls")==a);
 check(incomingOwner(b,"children")==a);
 check(!incomingOwner(set,"children"));
 check(!controlTreeProjectionIncomingOwnerCandidate(p,set,"controls",group,
  [&](auto* e){return e==set?10u:11u;}));
 p.graph.strings.push_back("visible");
 Attribute groupVisible{};groupVisible.name=static_cast<uint32_t>(p.graph.strings.size()-1);groupVisible.type=4;
 Value ownVisible;ownVisible.wire={0};groupVisible.values={ownVisible};p.graph.elements[1].attributes.push_back(groupVisible);
 ownVisible.wire={1};groupVisible.values={ownVisible};p.graph.elements[2].attributes.push_back(groupVisible);
 auto projectedVisibility=[&](bool differentOwner){return controlTreeProjectionVisibleCandidate(
  p,b,group,[&](auto* e){return differentOwner&&e==a?11u:10u;},
  [&](auto* e)->size_t{return e==a?1:2;},
  [](auto*)->uint8_t{throw std::runtime_error("unexpected missing visibility fixture");});};
 check(projectedVisibility(false)==0);
 check(projectedVisibility(true)==1);
 p.graph.elements[1].attributes.back().values[0].wire[0]=1;
 check(projectedVisibility(false)==1);
 p.graph.elements[1].attributes.back().values[0].wire[0]=0;
 check(p.scene.setArrayReferenceRange(childAttribute->token,0,{0xffffffffu},0,0xffffffffu,[](uint32_t,uint32_t){return true;}));
 check(projectedVisibility(false)==1); // Live removal disconnects hidden ancestor.
 check(p.scene.setArrayReferenceRange(childAttribute->token,0,{p.elementHandles[2]},0,0xffffffffu,[](uint32_t,uint32_t){return true;}));
 check(projectedVisibility(false)==0);
 p.graph.elements[1].attributes.pop_back();p.graph.elements[2].attributes.pop_back();p.graph.strings.pop_back();
 check(tree.groups.empty()); // Referenced groupB is empty and skipped.
 check(tree.foreground(treeRoot)==0x55443322u);
 for(auto& row:tree.leaves.front().nodes)check(tree.foreground(row)==0x88776655u);
 int resolveCalls=0,parentCalls=0,targetCalls=0;
 auto decorate=[&](int kind,bool animationType,bool transformType,decltype(a) cameraParent,decltype(a) target){
  return controlModelDisplayDecorationCandidate("control",kind,
   [&](){++resolveCalls;return set;},[&](auto*){return animationType;},
   [](auto*,const char* base){return std::string(base)+" (capture fixture)";},
   [&](auto*){return transformType;},[&](){++parentCalls;return cameraParent;},
   [&](auto*){++targetCalls;return target;});
 };
 check(decorate(4,false,true,a,a)=="control (work camera parent)");
 check(resolveCalls==2&&parentCalls==1&&targetCalls==1);
 resolveCalls=parentCalls=targetCalls=0;
 check(decorate(6,false,true,a,a)=="control"&&resolveCalls==1&&parentCalls==0&&targetCalls==0);
 check(decorate(4,false,true,nullptr,a)=="control"&&targetCalls==0);
 resolveCalls=parentCalls=targetCalls=0;
 check(decorate(4,true,true,a,a)=="control (capture fixture)");
 check(resolveCalls==1&&parentCalls==0&&targetCalls==0);
 check(decorate(4,false,true,a,b)=="control");
 check(std::string(tree.baseLabelForNode(treeRoot))=="set");
 check(std::string(tree.baseLabelForNode(tree.leaves.front().nodes[0]))=="set");
 check(std::string(tree.baseLabelForNode(tree.leaves.front().nodes[8]))=="x");
 tree.records.front().selectionBits=0x12;
 check(tree.selectionState(treeRoot)==1); // Nonrecursive cached read before refresh.
 check(tree.refreshSelectionState(&treeRoot)==3&&tree.selectionState(treeRoot)==3);
 check(tree.selectionState(tree.leaves.front().nodes[8])==2);
 check(tree.selectionState(tree.leaves.front().nodes[7])==1);
 check(tree.selectionState(tree.leaves.front().nodes[5])==3);
 check(tree.selectionState(tree.leaves.front().nodes[1])==3);
 auto fullBackground=tree.selectionBackground(tree.leaves.front().nodes[8]);
 auto partialBackground=tree.selectionBackground(treeRoot);
 check(fullBackground&&fullBackground->brushStyle==1&&fullBackground->darkFactor==0);
 check(partialBackground&&partialBackground->brushStyle==1&&partialBackground->darkFactor==135);
 check(!tree.selectionBackground(tree.leaves.front().nodes[7]));
 check(!controlSelectionBackgroundCandidate(0));
 check(tree.refreshSelectionState(nullptr)==0);
 for(auto& node:tree.leaves.front().nodes){
  uint32_t available=999,selected=999;
  check(tree.elementForNode(node,&available,&selected)==set);
  check(available==(knownControlLeafMaskCandidate(node.kind)&0x7e));
  check(selected==(knownControlLeafMaskCandidate(node.kind)&0x12));
  const uint32_t expectedColumn1=node.kind==4?0x7cu:(node.kind==5||node.kind==9?0x3cu:0u);
  check(tree.flagsForNode(&node,0,0,true)==0x21u);
  check(tree.flagsForNode(&node,0,1,true)==expectedColumn1);
  check(tree.flagsForNode(&node,0,2,true)==0x20u);
  check(tree.flagsForNode(&node,0,3,true)==0x20u);
 }
 check(tree.flagsForNode(nullptr,0,1,true)==0x21u);
 check(tree.flagsForNode(nullptr,-1,1,true)==0u);
 check(tree.flagsForNode(&treeRoot,0,3,true)==0x30u);
 check(tree.elementForNode(treeRoot)==set);
 auto& axis=tree.leaves.front().nodes[8];
 axis.inheritedFlag=1;
 check(tree.foreground(axis)==0x40776655u);
 axis.inheritedFlag=0;
 check(std::string(controlNodeBaseLabelCandidate(axis.kind,tree.elementForNode(axis)!=nullptr,"ignored"))=="x");
 tree.records.front().elementHandle=0xffffffffu;
 check(tree.foreground(axis)==0xffffffffu);
 check(std::string(tree.baseLabelForNode(axis))=="<ERROR - MISSING ELEMENT>");
 check(tree.flagsForNode(&tree.leaves.front().nodes[0],0,1,true)==0u);
 uint32_t available=999,selected=999;
 check(!tree.elementForNode(axis,&available,&selected)&&available==2&&selected==2);
 check(std::string(controlNodeBaseLabelCandidate(axis.kind,false,"ignored"))=="<ERROR - MISSING ELEMENT>");
 tree.records.front().elementHandle=set->record.handle;
 ControlGroupSelectionNodeCandidate unbound{};unbound.kind=4;
 available=999;selected=999;
 check(!tree.elementForNode(unbound,&available,&selected)&&available==999&&selected==999);
 check(p.scene.setArrayReferenceRange(controlAttribute->token,0,{p.elementHandles[2]},0,0xffffffffu,[](uint32_t,uint32_t){return true;}));
 ControlTreeProjectionPopulationCandidate<decltype(p),TreeSemantics> genericTree(p,semantics);
 ControlGroupSelectionNodeCandidate genericRoot{};genericRoot.kind=2;genericRoot.elementHandle=set->record.handle;
 genericTree.populate(genericRoot);
 check(genericTree.recordCount()==1&&genericTree.records.front().availabilityBits==0x80);
 check(controlChildCountCandidate(genericRoot)==1&&controlChildCountCandidate(*genericRoot.firstChild)==0);
 check(genericTree.elementForNode(*genericRoot.firstChild)==b);
 check(genericTree.foreground(*genericRoot.firstChild)==0x80ffffffu);
 check(!incomingOwner(set,"controls"));
 check(incomingOwner(b,"controls")==a);
 check(genericTree.flagsForNode(genericRoot.firstChild,0,1,true)==0);
 auto indexFlags=[&](int row,int column,bool identity,bool present){
  return animationSetProjectionIndexFlagsCandidate(p,row,column,identity,present,2,
   p.elementHandles[0],group,animation,[]{return false;});
 };
 check(indexFlags(0,0,true,true)==0x21u);
 check(indexFlags(-1,0,true,true)==0u);
 check(indexFlags(0,-1,true,true)==0u);
 check(indexFlags(0,0,false,true)==0u);
 check(indexFlags(0,3,false,false)==0u);
 // The original custom branch may add flags even when Qt's base is zero.
 check(indexFlags(0,3,false,true)==0x10u);
 auto flags=[&](int kind,uint32_t h,int column){return animationSetProjectionFlagsCandidate(p,0x21u,true,kind,h,column,group,animation,[]{return false;});};
 check(flags(2,p.elementHandles[0],3)==0x30u);
 check(lookup(2,p.elementHandles[0])==a);check(lookup(3,p.elementHandles[1])==a);
 check(!lookup(2,p.elementHandles[1]));check(!lookup(3,p.elementHandles[0]));
 check(!lookup(1,p.elementHandles[0]));check(!lookup(2,0xffffffffu));
 auto token=set->attributes->token;
 check(p.scene.setScalarReference(token,p.elementHandles[2],0,0xffffffffu,[](uint32_t,uint32_t){return true;}));
 check(lookup(2,p.elementHandles[0])==b);
 check(flags(2,p.elementHandles[0],3)==0x30u);
 check(p.graph.elements[0].attributes[0].values[0].reference==1);
 check(p.scene.setScalarReference(token,0xffffffffu,0,0xffffffffu,[](uint32_t,uint32_t){return true;}));
 check(!lookup(2,p.elementHandles[0]));
 check(flags(2,p.elementHandles[0],3)==0x20u);
 check(flags(1,p.elementHandles[0],3)==0x70u);
 check(flags(2,p.elementHandles[0],0)==0x21u);
 check(animationSetProjectionFlagsCandidate(p,0x21u,false,2,p.elementHandles[0],3,
 group,animation,[]{return false;})==0x21u);
 std::cout<<"x64=1 initial_group=1 live_mutation=1 stale_wire_ignored=1 cleared_group=1 type_and_kind_rejection=1 live_ui_flags=1 original_runtime_parity=unverified\n";
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
