#pragma once
#include "element_callback_mask_candidate.h"
#include "element_callback_list_collect_candidate.h"
#include <cassert>
// Normal acyclic list path of103603f0. Empty snapshot stores required; caller
// provides native pointer storage/paired payload type and callback ABI bindings.
template<class Element,class Adapter,class Filtered,class Callbacks,class Values>
inline void dispatchElementPostUpdateCandidate(Element* element,Adapter& a,
    Filtered& filtered,Callbacks& callbacks,Values& values) {
    assert(filtered.count()==0 && callbacks.count()==0 && values.count()==0);
    if(a.state(element)&0x10u) {
        for(auto* node=a.filteredHead(element);node;node=a.filteredNext(node))
            if(elementCallbackMaskSelectedCandidate(a.requiredMask(node),a.currentMask(element)))
                filtered.append(a.filteredCallback(node));
    }
    collectRotateElementCallbackListCandidate(a.dirtyHead(element),a.retainedHead(element),
        [&](auto callback){callbacks.append(callback);},[&](auto value){values.append(value);});
    assert(callbacks.count()==values.count());
    for(int i=0;i<filtered.count();++i)
        a.invokeFiltered(filtered.data()[i],element,a.currentMask(element)&3u);
    for(int i=0;i<callbacks.count();++i)
        a.invokeValue(callbacks.data()[i],values.data()[i]);
}
