#pragma once
#include <cstdint>
struct StudioMaterialSharedBufferControlCandidate{uint32_t users=0,selector=0,diagnostic=0;};
// getGame/equal are supplied to preserve native short-circuit read order.
// Init returns ignored in native; callers must serialize this shared state.
template<class Game,class Equal,class Initialize>void studioMaterialSharedBuffersAcquireCandidate(StudioMaterialSharedBufferControlCandidate& shared,Game getGame,Equal equal,Initialize initialize){
 if(!shared.users){uintptr_t reserve=0x226000;
 if(equal("infested",getGame())||equal("swarm",getGame()))reserve=0x44c000;
 else if(equal("portal2",getGame()))reserve=0x672000;
 initialize(0,reserve,0x8000,0,32);initialize(1,reserve,0x8000,0,32);shared.selector=shared.diagnostic=0;}
 ++shared.users;
}

inline bool studioMaterialGameKeyEqualCandidate(const char* left,const char* right){
 auto fold=[](unsigned char value){return value>='A'&&value<='Z'?unsigned(value+32):unsigned(value);};
 while(*left&&*right){if(fold(static_cast<unsigned char>(*left))!=fold(static_cast<unsigned char>(*right)))return false;++left;++right;}return *left==*right;
}
