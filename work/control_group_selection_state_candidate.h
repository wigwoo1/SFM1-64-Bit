#pragma once
#include <cstdint>
#include <stdexcept>
//1008d3b0: resolution precedes static labels, including for axis leaves.
inline const char* controlNodeBaseLabelCandidate(int32_t kind,bool elementResolved,
 const char* elementName){
 if(!elementResolved)return "<ERROR - MISSING ELEMENT>";
 if(kind<=4)return elementName?elementName:"";
 static constexpr const char* labels[]={"pos","x","y","z","rot","x","y","z"};
 if(kind>12)throw std::out_of_range("Unrecovered control label kind");
 return labels[kind-5];
}
// Restricted to kinds independently observed in the recovered leaf builder.
inline uint32_t knownControlLeafMaskCandidate(int32_t kind){
 static constexpr uint32_t masks[]={0xffffffffu,0x0e,2,4,8,0x70,0x10,0x20,0x40};
 if(kind<4||kind>12)throw std::out_of_range("Unrecovered control leaf kind");
 return masks[kind-4];
}
// Portable structure, not original x86 object layout. Raw state codes from1008c190.
struct ControlGroupSelectionNodeCandidate {
 int32_t kind=0;
 uint8_t inheritedFlag=0;
 uint32_t elementHandle=0xffffffffu;
 uint32_t aggregate=0;
 const uint32_t* controlSelectionBits=nullptr;
 ControlGroupSelectionNodeCandidate* parent=nullptr;
 ControlGroupSelectionNodeCandidate* firstChild=nullptr;
 ControlGroupSelectionNodeCandidate* nextSibling=nullptr;
};
// Recovered1008c6c0/1008c6e0. Valid acyclic sibling chains are assumed by
// the original routines as well; lifetime and structural edits belong to model.
inline uint32_t controlChildCountCandidate(const ControlGroupSelectionNodeCandidate& parent){
 uint32_t count=0;
 for(auto* child=parent.firstChild;child;child=child->nextSibling)++count;
 return count;
}
//1008be70: follows sibling links while decrementing the requested ordinal.
inline ControlGroupSelectionNodeCandidate* controlChildAtCandidate(
 const ControlGroupSelectionNodeCandidate& parent,int32_t ordinal){
 auto* child=parent.firstChild;
 auto remaining=static_cast<uint32_t>(ordinal);
 while(child){
  if(remaining==0)return child;
  child=child->nextSibling;
  --remaining;
 }
 return nullptr;
}
inline int32_t controlChildRowCandidate(const ControlGroupSelectionNodeCandidate& parent,
 const ControlGroupSelectionNodeCandidate* target){
 uint32_t row=0;
 for(auto* child=parent.firstChild;child;child=child->nextSibling,++row)
  if(child==target)return static_cast<int32_t>(row);
 return -1;
}
inline void initializeControlLeafCandidate(ControlGroupSelectionNodeCandidate& node,
 int32_t kind,const uint32_t* selectionBits,ControlGroupSelectionNodeCandidate* parent){
 node.kind=kind;
 node.inheritedFlag=parent?parent->inheritedFlag:0;
 node.aggregate=0;
 node.elementHandle=0xffffffffu;
 node.controlSelectionBits=selectionBits;
 node.parent=parent;
 node.firstChild=nullptr;
 node.nextSibling=parent?parent->firstChild:nullptr;
 if(parent)parent->firstChild=&node;
}
//1008d100: group nodes prepend just like leaves, but retain a group handle.
// Caller supplies the current handle; original handle wrapper ownership is separate.
inline void initializeControlGroupNodeCandidate(ControlGroupSelectionNodeCandidate& node,
 uint32_t groupHandle,ControlGroupSelectionNodeCandidate* parent){
 initializeControlLeafCandidate(node,3,nullptr,parent);
 node.elementHandle=groupHandle;
}
// Portable ownership remains with caller. Index zero is kind4; remaining nodes
// correspond to recovered constructor call order, not display order.
inline void initializeTransformControlLeavesCandidate(
 ControlGroupSelectionNodeCandidate (&nodes)[9], const uint32_t* selectionBits,
 ControlGroupSelectionNodeCandidate* parent){
 initializeControlLeafCandidate(nodes[0],4,selectionBits,parent);
 initializeControlLeafCandidate(nodes[1],9,selectionBits,&nodes[0]);
 initializeControlLeafCandidate(nodes[2],12,selectionBits,&nodes[1]);
 initializeControlLeafCandidate(nodes[3],11,selectionBits,&nodes[1]);
 initializeControlLeafCandidate(nodes[4],10,selectionBits,&nodes[1]);
 initializeControlLeafCandidate(nodes[5],5,selectionBits,&nodes[0]);
 initializeControlLeafCandidate(nodes[6],8,selectionBits,&nodes[5]);
 initializeControlLeafCandidate(nodes[7],7,selectionBits,&nodes[5]);
 initializeControlLeafCandidate(nodes[8],6,selectionBits,&nodes[5]);
}
template<class MaskForKind>
uint32_t readControlSelectionCandidate(const ControlGroupSelectionNodeCandidate&node,const MaskForKind&maskForKind){
 if(node.kind<4)return node.aggregate;
 if(!node.controlSelectionBits)return 0;
 const auto mask=maskForKind(node.kind);
 const auto selected=*node.controlSelectionBits&mask;
 if(selected==0)return 1;
 return selected==mask?2u:3u;
}
// Caller supplies verified kind-to-mask mapping; domain is not invented here.
template<class MaskForKind>
uint32_t updateControlGroupSelectionCandidate(ControlGroupSelectionNodeCandidate* node,const MaskForKind&maskForKind){
 if(!node)return 0;
 if(node->kind>=4){
  return readControlSelectionCandidate(*node,maskForKind);
 }
 uint32_t state=0;
 for(auto*child=node->firstChild;child;child=child->nextSibling)
  state|=updateControlGroupSelectionCandidate(child,maskForKind);
 node->aggregate=state;
 return state;
}
