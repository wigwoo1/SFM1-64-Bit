#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "reference_owned_dmx_projection.h"
#include "animation_set_projection_group_candidate.h"
using UiProjection = RecoveredOwnedDmxReferenceProjectionCandidate<Graph>;
RecoveredOwnedSceneElementCandidate* compileProjectionGroup(UiProjection& projection,
 std::int32_t kind,std::uint32_t handle,
 bool(*isGroup)(RecoveredOwnedSceneElementCandidate*),
 bool(*isAnimationSet)(RecoveredOwnedSceneElementCandidate*)) {
 static_assert(sizeof(void*)==8,"x64 required");
 return animationSetProjectionGroupCandidate(projection,kind,handle,isGroup,isAnimationSet);
}
