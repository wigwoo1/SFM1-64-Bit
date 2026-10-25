#pragma once
#include "ifm_bone_world_palette_candidate.h"
// Stable ordered reverse-reference contract for104D2940/10223170(kind0).
// File identity and native reference order are inputs, never guessed globally.
struct IfmBoneDagReferenceCandidate {
    bool attributePresent=true,ownerPresent=true,ownerDagTyped=true,nameMatches=true,sameFile=true;
    bool overridesDisabled=false,overridePresent=false,overrideDagTyped=true;
    RecoveredMatrix3x4 world{};
};
struct IfmBoneTransformReferencesCandidate {bool present=true,transformTyped=true;std::vector<IfmBoneDagReferenceCandidate>references;};
struct IfmBonePrecomputeCandidate {
    std::vector<uint32_t>selectedReferences;
    std::vector<std::optional<RecoveredMatrix3x4>>palette;
};
inline IfmBonePrecomputeCandidate ifmBonePrecomputeCandidate(
    bool modelResolved,bool gameModelTyped,const std::vector<IfmBoneTransformReferencesCandidate>&bones){
    if(bones.size()>256)throw std::runtime_error("IFM precomputed bone count");
    IfmBonePrecomputeCandidate out;out.selectedReferences.assign(bones.size(),UINT32_MAX);out.palette.resize(bones.size());
    if(!modelResolved||!gameModelTyped)return out;
    for(size_t i=0;i<bones.size();++i){const auto&b=bones[i];if(!b.present||!b.transformTyped)continue;
        for(size_t j=0;j<b.references.size();++j){const auto&r=b.references[j];
            if(!r.attributePresent||!r.ownerPresent||!r.ownerDagTyped||!r.nameMatches||!r.sameFile)continue;
            out.selectedReferences[i]=uint32_t(j);
            // Selection precedes this gate. A disabled/no-parent first match
            // does NOT cause a search for a later eligible reference.
            if(!r.overridesDisabled&&r.overridePresent&&r.overrideDagTyped){checkIfmMatrixFiniteCandidate(r.world);out.palette[i]=r.world;}
            break;
        }
    }return out;
}
