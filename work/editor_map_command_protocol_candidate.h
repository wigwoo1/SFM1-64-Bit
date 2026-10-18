#pragma once
// 102a2390 protocol. Adapter owns exact bounded-string and engine contracts.
template<class Adapter>
inline bool requestEditorMapCommandCandidate(const char* requested,Adapter& a) {
    if(a.pending())return false;
    if(!requested)requested=a.currentMap();
    auto name=a.extractMapBaseName(requested);
    if(!a.validateMap(name))return false;
    a.setPending(true);
    auto command=a.formatMapCommand(name);
    a.dispatchCommand(command);
    a.engineAfterCommand();
    a.refreshEditor();
    return true;
}
