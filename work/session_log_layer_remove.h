#pragma once
// Graph-array portion of 0x10502040 -> 0x103885E0 -> 0x103848D0.
// Native undo, handle-reference and attribute notifications remain pending.
template<class Release> inline bool removeSessionLogLayerReference(Graph&graph,uint32_t log,Release release){
 auto*layers=runtimeAttribute(graph,log,"layers");
 if(!layers||layers->type!=15)throw std::runtime_error("invalid log layers array");
 if(layers->values.empty())return false;
 release(layers->values.back().reference);
 layers->values.pop_back();return true;
}
inline bool removeSessionLogLayerReference(Graph&graph,uint32_t log){return removeSessionLogLayerReference(graph,log,[](uint32_t){});}
