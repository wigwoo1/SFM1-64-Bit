#include "graph_current_selection_candidate.h"
#include "graph_attribute_access_candidate.h"
#include <cassert>
#include <string>
#include <vector>
#include <iostream>
struct Attribute{Attribute* next;std::uintptr_t name;unsigned type,handle;};
struct Element{Attribute* head;std::uintptr_t type;};struct Child{unsigned handle=0;};
struct Adapter{
 std::string settingsName="settings",graphName="graphEditorState",settingsType="settings-type",graphType="graph-type";
 Attribute settingsAttr{},graphAttr{};Element root{},settings{},graph{};Child child;
 GraphAttributeDefaultCandidate wrapperDefault,storageDefault;unsigned scratch=0;std::vector<unsigned> resolved;
 Adapter(){settingsAttr={nullptr,reinterpret_cast<std::uintptr_t>(settingsName.c_str()),1,1};graphAttr={nullptr,reinterpret_cast<std::uintptr_t>(graphName.c_str()),1,2};root={&settingsAttr,0};settings={&graphAttr,reinterpret_cast<std::uintptr_t>(settingsType.c_str())};graph={nullptr,reinterpret_cast<std::uintptr_t>(graphType.c_str())};}
 Child* globalChild344(){return &child;}unsigned childHandle80(Child* c){return c->handle;}Element* nullSelection(){return nullptr;}
 Element* resolveModelSlot18(unsigned h){resolved.push_back(h);switch(h){case 0:return &root;case 1:return &settings;case 2:return &graph;default:return nullptr;}}
 std::uintptr_t internModelSlot50(const char* name){return reinterpret_cast<std::uintptr_t>((std::string(name)=="settings"?settingsName:graphName).c_str());}
 Attribute* head30(Element* e){return e->head;}Attribute* next0(Attribute* a){return a->next;}std::uintptr_t symbol14(Attribute* a){return a->name;}
 Attribute* findAttribute1035dd50(Element* e,const char* name){return graphFindAttributeCandidate(e,name,*this);}
 unsigned type10(Attribute* a){return a->type;}unsigned* data4(Attribute* a){return &a->handle;}
 unsigned canConvert10362e50(Attribute*){return 0;}void convert103630a0(Attribute*,unsigned*){assert(false);}
 unsigned* handleStorage10002780(Attribute* a){return graphAttributeHandleStorageCandidate(a,storageDefault,scratch,*this);}
 std::uintptr_t liveTypeSymbol(std::uintptr_t tag){return reinterpret_cast<std::uintptr_t>((tag==0x10e9a684?settingsType:graphType).c_str());}
 unsigned elementTypeSlot4(Element* e,std::uintptr_t s){return e->type==s;}
 Element* lookup10002830(Element* e){return graphTypedAttributeCandidate(e,"settings",0x10e9a684,wrapperDefault,*this);}
 Element* lookup102c4420(Element* e){return graphTypedAttributeCandidate(e,"graphEditorState",0x10eff4f4,wrapperDefault,*this);}
};
int main(){static_assert(sizeof(void*)==8);Adapter a;assert(graphCurrentSelectionCandidate(a)==&a.graph);assert((a.resolved==std::vector<unsigned>{0,0,1,2}));
 a.graphAttr.handle=999;assert(!graphCurrentSelectionCandidate(a));a.graphAttr.handle=2;a.graph.type=0;assert(!graphCurrentSelectionCandidate(a));a.graph.type=reinterpret_cast<std::uintptr_t>(a.graphType.c_str());a.root.head=nullptr;assert(!graphCurrentSelectionCandidate(a) && a.resolved.back()==0xffffffff);
 std::cout<<"native x64 graph state composition: linked attributes, interned names, handle chain, type filters and missing state passed\n";
}
