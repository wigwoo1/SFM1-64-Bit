#pragma once
#include "control_group_selection_state_candidate.h"
#include "control_model_group_lookup_candidate.h"
#include "control_model_item_flags_candidate.h"

// Composition of recovered model operations, not a Qt model or production graph.
// Graph provides resolve(handle), isGroup(element), isAnimationSet(element),
// rootGroup(element), and isTransformControl(node). Preserve lookup side effects.
template<class Element,class Graph>
std::uint32_t animationSetControlFlagsCandidate(std::uint32_t base,
    const ControlGroupSelectionNodeCandidate* node,std::int32_t column,Graph& graph) {
    return controlModelItemFlagsCandidate(base,node!=nullptr,column,node?node->kind:0,
        [&]() {
            return controlModelGroupLookupCandidate<Element>(node->kind,node->elementHandle,
                [&](std::uint32_t handle){return graph.resolve(handle);},
                [&](Element* element){return graph.isGroup(element);},
                [&](Element* element){return graph.isAnimationSet(element);},
                [&](Element* element){return graph.rootGroup(element);}) != nullptr;
        },[&](){return graph.isTransformControl(*node);});
}
