#pragma once
#include "session_runtime_handles.h"
#include "graph_current_selection_candidate.h"
#include "graph_typed_attribute_candidate.h"
#include <map>
#include <set>
// Projection adapter for immutable loaded DMX data. Runtime linked attributes,
// derived type inheritance and type conversion are not represented by Graph.
struct SessionGraphStateLookupCandidate{
 Graph& graph;SessionRuntimeHandles& handles;std::uint32_t rootHandle;
 GraphAttributeDefaultCandidate defaults;
 std::map<Attribute*,std::uint32_t> referenceStorage;std::set<std::string> symbols;
 SessionGraphStateLookupCandidate(Graph& g,SessionRuntimeHandles& h,std::uint32_t root):graph(g),handles(h),rootHandle(root){}
 SessionGraphStateLookupCandidate* globalChild344(){return this;}
 std::uint32_t childHandle80(SessionGraphStateLookupCandidate*){return rootHandle;}
 Element* nullSelection(){return nullptr;}
 Element* resolveModelSlot18(std::uint32_t handle){return handles.resolve(graph,handle);}
 Attribute* findAttribute1035dd50(Element* e,const char* name){for(auto& a:e->attributes)if(graph.strings.at(a.name)==name)return &a;return nullptr;}
 std::uint32_t* handleStorage10002780(Attribute* a){
  if((a->type&31)!=1 || a->values.size()!=1)throw std::runtime_error("graph state reference conversion not implemented");
  auto& value=referenceStorage[a];value=handles.fromReference(a->values[0]);return &value;
 }
 std::uintptr_t liveTypeSymbol(std::uintptr_t tag){auto name=tag==0x10e9a684?"DmElement":"DmeGraphEditorState";return reinterpret_cast<std::uintptr_t>(symbols.emplace(name).first->c_str());}
 unsigned elementTypeSlot4(Element* e,std::uintptr_t symbol){return graph.strings.at(e->type)==reinterpret_cast<const char*>(symbol);}
 Element* lookup10002830(Element* e){return graphTypedAttributeCandidate(e,"settings",0x10e9a684,defaults,*this);}
 Element* lookup102c4420(Element* e){return graphTypedAttributeCandidate(e,"graphEditorState",0x10eff4f4,defaults,*this);}
 Element* lookup(){return graphCurrentSelectionCandidate(*this);}
};
