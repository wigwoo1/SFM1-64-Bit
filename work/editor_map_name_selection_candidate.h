#pragma once
#include "map_base_name_candidate.h"
// 102a2210: first resolution is a guard; native routine resolves again before
// reading the name. Caller must preserve valid object lifetime across service calls.
template<class Adapter>
inline const char* projectMapNameCandidate(Adapter& a) {
    if(!a.resolvePrimary())return a.emptyName();
    auto* primary=a.resolvePrimary();
    const char* name=a.mapName(primary);
    if(name && *name)return name;
    if(auto* fallback=a.fallbackObject())return a.mapName(fallback);
    return a.emptyName();
}
// 102a34c0 captures engine basename even when a supplied name will win.
template<class Adapter>
inline void selectEditorMapNameCandidate(const char* supplied,char* output,std::size_t capacity,Adapter& a) {
    char engineBase[256];extractMapBaseNameCandidate(a.engineMap(),engineBase,sizeof(engineBase));
    const char* selected=supplied;
    if(!selected || !*selected) {
        selected=a.projectMap();
        if(!selected || !*selected)selected=engineBase;
    }
    extractMapBaseNameCandidate(selected,output,capacity);
}
