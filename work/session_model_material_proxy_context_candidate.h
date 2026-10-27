#pragma once
#include "session_render_scene_candidate.h"

// Revalidate against the graph used by this draw. A captured scene cannot be
// silently reused for a different GUID, asset, type or exceptional source path.
inline std::optional<MaterialModelProxyBindContextCandidate>sessionModelMaterialProxyContextCandidate(
        const Graph&graph,const SessionRenderModelCandidate&model){
    if(model.id>=graph.elements.size())return {};
    const auto&guid=graph.elements[model.id].id;
    const auto current=readSessionModelProxyContextCandidate(graph,model.id);
    if(!current.matches(model.id,guid,model.placement.asset))return {};
    return MaterialModelProxyBindContextCandidate::checked(model.proxyContext,model.id,guid,model.placement.asset);
}
