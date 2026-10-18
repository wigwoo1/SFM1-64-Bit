#pragma once
#include <cstdint>
// Deferred semantic selection. Geometry kernels identify a surface; the world
// owner resolves its material flags. Displacement index is the retained owned
// mesh ordinal, not a texture index or the original native name pointer.
enum class SourceCollisionSurfaceKindCandidate:uint32_t { Default,TextureInfo,RuntimeSurface,Displacement,BoxFace };
struct SourceCollisionSurfaceCandidate {
    SourceCollisionSurfaceKindCandidate kind=SourceCollisionSurfaceKindCandidate::Default;
    uint32_t index=0;
};
