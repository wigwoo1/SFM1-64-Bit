#pragma once
#include <vector>
#include <algorithm>
// Native 0x1035E530 prepends without checking duplicates; 0x1035F980 removes
// the first matching pointer. Logical vectors replace native pool-backed nodes.
template<class Listener>void addAttributeAttachmentListenerCandidate(std::vector<Listener*>&listeners,Listener*listener){listeners.insert(listeners.begin(),listener);}
template<class Listener>void removeAttributeAttachmentListenerCandidate(std::vector<Listener*>&listeners,Listener*listener){auto found=std::find(listeners.begin(),listeners.end(),listener);if(found!=listeners.end())listeners.erase(found);}
// Native 0x10360800 snapshots owner+0x54 linked listener pointers before invoking
// listener virtual slot0(attribute). Caller owns listener lifetimes through dispatch.
template<class Listener,class Callback>
void dispatchAttributeAttachmentListenersCandidate(bool suppressed,const std::vector<Listener*>&listeners,Callback callback){
 if(suppressed)return;const auto snapshot=listeners;for(auto*listener:snapshot)callback(listener);
}
