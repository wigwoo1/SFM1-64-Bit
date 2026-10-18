#include "control_tree_model_navigation_candidate.h"
#include "control_group_incoming_parent_candidate.h"
#include "control_group_visibility_candidate.h"
#include "control_tree_population_candidate.h"
#include "capture_device_slot_candidate.h"
#include "label_format_result_candidate.h"
#include "control_model_foreground_candidate.h"
#include <vector>
#include <deque>
#include <stdexcept>
#include <iostream>
struct PopulationFixture {
 struct Element{int handle;bool group,visible=true;std::vector<Element*> controls,children;};
 std::vector<Element*> records;
 std::deque<ControlGroupSelectionNodeCandidate> nodes;
 std::vector<Element*> elements;
 bool hidden=false;
 int overrideReads=0;
 Element* groupForNode(const ControlGroupSelectionNodeCandidate& n){
  for(auto* e:elements)if(static_cast<std::uint32_t>(e->handle)==n.elementHandle)return e;
  return nullptr;
 }
 int recordCount(){return static_cast<int>(records.size());}
 int controlCount(Element* e){return static_cast<int>(e->controls.size());}
 int childCount(Element* e){return static_cast<int>(e->children.size());}
 Element* controlAt(Element* e,int i){return e->controls.at(i);}
 Element* childAt(Element* e,int i){return e->children.at(i);}
 bool isControl(Element* e){return !e->group;}
 bool isGroup(Element* e){return e->group;}
 bool visible(Element* e){return e->visible;}
 bool showHiddenGroups(){++overrideReads;return hidden;}
 void addControl(Element* e,ControlGroupSelectionNodeCandidate* p){
  records.push_back(e);nodes.emplace_back();
  initializeControlLeafCandidate(nodes.back(),4,nullptr,p);
  nodes.back().elementHandle=static_cast<std::uint32_t>(e->handle);
 }
 ControlGroupSelectionNodeCandidate* addGroup(Element* e,ControlGroupSelectionNodeCandidate* p){
  nodes.emplace_back();initializeControlGroupNodeCandidate(nodes.back(),e->handle,p);
  return &nodes.back();
 }
};
int main(){
 static_assert(sizeof(void*)==8,"x64 required");
 auto check=[](bool result){if(!result)throw std::runtime_error("navigation mismatch");};
 int groupObject=0,controlObject=0,groupCalls=0,controlCalls=0,ownerCalls=0;
 auto foreground=[&](int kind,bool inherited,bool hasGroup,bool hasControl,bool hasOwner){
  return controlModelForegroundCandidate(kind,inherited,
   [&]()->int*{++groupCalls;return hasGroup?&groupObject:nullptr;},
   [](int*){return 0x7f332211u;},
   [&]()->int*{++controlCalls;return hasControl?&controlObject:nullptr;},
   [&](int*)->int*{++ownerCalls;return hasOwner?&groupObject:nullptr;},
   [](int*){return 0x9f665544u;});
 };
 check(foreground(1,false,true,true,true)==0xff808080u&&groupCalls==0&&controlCalls==0);
 check(foreground(1,true,true,true,true)==0x40808080u);
 check(foreground(3,false,true,true,true)==0x7f332211u&&controlCalls==0);
 check(foreground(4,false,false,false,true)==0xffffffffu&&ownerCalls==0);
 check(foreground(4,false,false,true,false)==0x80ffffffu);
 check(foreground(4,false,false,true,true)==0x9f665544u);
 check(foreground(4,true,false,true,true)==0x40665544u);
 char formatBuffer[8];bool truncated=true;
 formatBuffer[7]='x';
 check(labelFormatResultCandidate(formatBuffer,8,&truncated,[](char*,int){return 7;})==7&&!truncated&&formatBuffer[7]=='x');
 check(labelFormatResultCandidate(formatBuffer,8,&truncated,[](char*,int){return 8;})==8&&truncated&&formatBuffer[7]==0);
 formatBuffer[7]='x';
 check(labelFormatResultCandidate(formatBuffer,8,nullptr,[](char*,int){return -1;})==8&&formatBuffer[7]==0);
 int fixedRecord=1,dynamicRecord=2,fixedReads=0,dynamicReads=0;
 const uint32_t slots[]={0xffff0003u,0x12348005u};
 auto slot=[&](int index){return captureDeviceSlotCandidate(index,[]{return 2;},
  [&](int i){return slots[i];},[&](uint32_t i)->int*{++fixedReads;check(i==3);return &fixedRecord;},
  [&](uint32_t i)->int*{++dynamicReads;check(i==5);return &dynamicRecord;});};
 check(slot(0)==&fixedRecord&&slot(1)==&dynamicRecord);
 check(!slot(-1)&&!slot(2)&&fixedReads==1&&dynamicReads==1);
 struct CaptureRecord{uint32_t handle;};
 CaptureRecord captureRecords[]={{1},{2},{2}};
 int captureElement=0,otherElement=0,countReads=0,slotReads=0,typeReads=0;
 int liveCount=3;bool shrink=false;
 auto findCapture=[&](int* sought){return findCaptureAnimationSetCandidate(sought,
  [&](){++countReads;return liveCount;},
  [&](int i){++slotReads;if(shrink)liveCount=1;return &captureRecords[i];},
  [](auto* r){return r->handle;},
  [&](uint32_t h)->int*{return h==2?&captureElement:&otherElement;},
  [&](int* e){++typeReads;return e==&captureElement;});};
 check(findCapture(&captureElement)==1&&slotReads==2&&countReads==2);
 countReads=slotReads=typeReads=0;
 check(findCapture(nullptr)==-1&&countReads==0&&slotReads==0&&typeReads==0);
 shrink=true;
 check(findCapture(&captureElement)==-1&&slotReads==1&&countReads==2);
 PopulationFixture::Element c1{1,false},c2{2,false},c3{3,false},
  populated{10,true},empty{11,true},hiddenGroup{12,true,false},source{20,true};
 populated.controls={&c3};hiddenGroup.controls={&c1};
 source.controls={&c1,nullptr,&c2};
 source.children={&populated,&empty,&hiddenGroup,nullptr,&c1};
 PopulationFixture population;population.elements={&source,&populated,&empty,&hiddenGroup};
 ControlGroupSelectionNodeCandidate sourceNode{};sourceNode.elementHandle=20;
 populateControlTreeCandidate(&sourceNode,population);
 check(population.records.size()==3&&population.records[0]==&c2&&population.records[1]==&c1);
 check(sourceNode.aggregate==1&&population.overrideReads==1);
 check(controlChildCountCandidate(sourceNode)==3);
 check(controlChildAtCandidate(sourceNode,0)->elementHandle==10);
 check(controlChildAtCandidate(sourceNode,1)->elementHandle==1);
 check(controlChildAtCandidate(sourceNode,2)->elementHandle==2);
 check(controlChildAtCandidate(sourceNode,0)->aggregate==1);
 PopulationFixture hiddenPopulation;hiddenPopulation.elements=population.elements;hiddenPopulation.hidden=true;
 ControlGroupSelectionNodeCandidate hiddenRoot{};hiddenRoot.elementHandle=20;
 populateControlTreeCandidate(&hiddenRoot,hiddenPopulation);
 check(controlChildCountCandidate(hiddenRoot)==4&&hiddenPopulation.records.size()==4);
 check(controlChildAtCandidate(hiddenRoot,0)->elementHandle==10);
 check(controlChildAtCandidate(hiddenRoot,1)->elementHandle==12);
 ControlGroupSelectionNodeCandidate root{},nodes[9];
 int model=0,otherModel=0;
 auto parentFilter=[&](unsigned flags,unsigned symbol,bool same,unsigned ownerId,
 unsigned restriction,bool typed=true){
  return controlGroupIncomingParentCandidate(true,static_cast<std::uint16_t>(flags),
   &root,symbol,7,same,ownerId,10,restriction,[&](auto*){return typed;});
 };
 check(parentFilter(0x1000,7,true,10,0)==&root);
 check(!parentFilter(0x1000,7,true,10,1));
 check(parentFilter(0,7,true,10,1)==&root);
 check(!parentFilter(0,7,true,10,2));
 check(parentFilter(0x800,7,true,10,2)==&root);
 check(!parentFilter(0x1800,7,true,10,2));
 check(!parentFilter(0,8,true,10,0));
 check(!parentFilter(0,7,true,11,0));
 check(parentFilter(0,7,false,11,0)==&root);
 check(!parentFilter(0,7,true,10,3));
 check(!parentFilter(0,7,true,10,0,false));
 int iteratorNextCalls=0;
 auto* firstOwner=firstIncomingControlGroupCandidate(&root,
  [](auto*){return 1u;},[&](unsigned i){++iteratorNextCalls;return i<3?i+1:0u;},
  [&](unsigned i){return i==1?parentFilter(0,8,true,10,0):parentFilter(0x1000,7,true,10,0);});
 check(firstOwner==&root&&iteratorNextCalls==1);
 struct VisibilityGroup{std::uint32_t identity;std::uint8_t visible;VisibilityGroup* parent;};
 VisibilityGroup ancestorGroup{10,1,nullptr},middleGroup{10,1,&ancestorGroup},
  childGroup{10,1,&middleGroup};
 std::vector<VisibilityGroup*> reads;
 auto selectParent=[&](VisibilityGroup* g){
  auto* p=g->parent;
  return controlGroupIncomingParentCandidate(p!=nullptr,0x1000,p,7,7,true,
   p?p->identity:0,g->identity,0,[](auto*){return true;});
 };
 auto readVisible=[&](VisibilityGroup* g){reads.push_back(g);return g->visible;};
 check(controlGroupVisibleCandidate(&childGroup,selectParent,readVisible)==1);
 check(reads.size()==3&&reads[0]==&ancestorGroup&&reads[2]==&childGroup);
 reads.clear();ancestorGroup.visible=0;
 check(controlGroupVisibleCandidate(&childGroup,selectParent,readVisible)==0);
 check(reads.size()==1&&reads[0]==&ancestorGroup);
 reads.clear();ancestorGroup.visible=1;middleGroup.visible=0;
 check(controlGroupVisibleCandidate(&childGroup,selectParent,readVisible)==0);
 check(reads.size()==2&&reads[1]==&middleGroup);
 reads.clear();middleGroup.identity=11;childGroup.visible=2;
 check(controlGroupVisibleCandidate(&childGroup,selectParent,readVisible)==2);
 check(reads.size()==1&&reads[0]==&childGroup);
 std::uint32_t selection=0;
 initializeTransformControlLeavesCandidate(nodes,&selection,&root);
 auto top=controlTreeIndexCandidate(&root,&model,0,3);
 check(top.valid()&&top.node==&nodes[0]);
 check(!controlTreeParentCandidate(&root,&model,top).valid());
 auto pos=controlTreeIndexCandidate(&root,&model,0,3,top);
 auto rot=controlTreeIndexCandidate(&root,&model,1,3,top);
 check(pos.node==&nodes[5]&&rot.node==&nodes[1]);
 for(int row=0;row<3;++row){
  auto axis=controlTreeIndexCandidate(&root,&model,row,3,pos);
  check(axis.node==&nodes[8-row]);
  auto parent=controlTreeParentCandidate(&root,&model,axis);
  check(parent.node==pos.node&&parent.row==0&&parent.column==3);
 }
 check(controlTreeRowCountCandidate(&root)==1);
 check(controlTreeRowCountCandidate(&root,top)==2);
 check(controlTreeRowCountCandidate(&root,pos)==3);
 check(controlTreeColumnCountCandidate()==4);
 check(!controlTreeIndexCandidate(&root,&model,-1,0).valid());
 check(!controlTreeIndexCandidate(&root,&model,1,0).valid());
 auto negativeColumn=controlTreeIndexCandidate(&root,&model,0,-1);
 check(negativeColumn.node==top.node&&!negativeColumn.valid());
 auto foreign=top;foreign.model=&otherModel;
 check(controlTreeRowCountCandidate(&root,foreign)==2);
 auto nullNode=top;nullNode.node=nullptr;
 check(controlTreeRowCountCandidate(&root,nullNode)==0);
 check(!controlTreeIndexCandidate(&root,&model,0,0,nullNode).valid());
 root.firstChild=nullptr;
 check(!controlTreeParentCandidate(&root,&model,pos).valid());
 ControlGroupSelectionNodeCandidate groupA{},groupB{},nested{},leaves[9];
 root.inheritedFlag=1;
 initializeControlGroupNodeCandidate(groupB,22,&root);
 initializeControlGroupNodeCandidate(groupA,11,&root);
 initializeControlGroupNodeCandidate(nested,33,&groupA);
 initializeTransformControlLeavesCandidate(leaves,&selection,&nested);
 check(controlTreeRowCountCandidate(&root)==2);
 auto groupIndex=controlTreeIndexCandidate(&root,&model,0,2);
 check(groupIndex.node==&groupA&&groupA.elementHandle==11);
 check(groupA.inheritedFlag==1&&nested.inheritedFlag==1);
 auto nestedIndex=controlTreeIndexCandidate(&root,&model,0,2,groupIndex);
 auto leafIndex=controlTreeIndexCandidate(&root,&model,0,2,nestedIndex);
 auto ancestor=controlTreeParentCandidate(&root,&model,leafIndex);
 check(ancestor.node==&nested&&ancestor.column==2);
 ancestor=controlTreeParentCandidate(&root,&model,ancestor);
 check(ancestor.node==&groupA&&ancestor.row==0&&ancestor.column==2);
 check(!controlTreeParentCandidate(&root,&model,ancestor).valid());
 std::cout<<"x64=1 transform_tree_navigation=passed column_preservation=passed original_runtime_parity=unverified\n";
}
