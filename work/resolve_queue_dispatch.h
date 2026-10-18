#pragma once
#include <vector>
#include <cstdint>
// Native1038F210 first queue: snapshot length; resolve/callback/clear per live handle.
// Reentrant appends verified: entries beyond initial count are discarded on clear.
// Removal/reordering during callbacks remains unverified. Owner supplies callbacks.
template<class Resolve,class Callback,class Clear>
inline void recoveredResolveQueueDispatchCandidate(std::vector<uint32_t>&queue,Resolve resolve,Callback callback,Clear clear){auto count=queue.size();for(size_t i=0;i<count;++i){auto element=resolve(queue.at(i));if(element){callback(element);clear(element);}}queue.clear();}
