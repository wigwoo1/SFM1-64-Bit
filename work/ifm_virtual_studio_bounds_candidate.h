#pragma once
#include "ifm_renderable_bounds_candidate.h"
#include "source_mdl_virtual_sequences_candidate.h"
// Preserve ROOT hull/view bounds while projecting resolved virtual sequence
// descriptors into the existing dispatcher's value table. Explicit groups/mode
// are required; include loading and native cache/header lifetime are not inferred.
inline IfmStudioBoundsCandidate ifmVirtualStudioBoundsCandidate(
    const std::vector<uint8_t>&root,const std::vector<SourceMdlSequenceMetadataCandidate>&groups,SourceSequenceNameCacheModeCandidate mode){
    auto result=readIfmMdlStudioBoundsCandidate(root);const auto metadata=readSourceMdlSequenceMetadataCandidate(root);
    if(groups.empty()||groups[0].sequences.size()!=metadata.sequences.size())throw std::runtime_error("virtual bounds root metadata count");
    for(size_t i=0;i<metadata.sequences.size();++i){const auto&a=metadata.sequences[i],&b=groups[0].sequences[i];if(a.label!=b.label||a.flags!=b.flags||a.activity!=b.activity||std::memcmp(a.bounds.data(),b.bounds.data(),24))throw std::runtime_error("virtual bounds root metadata mismatch");}
    const auto table=sourceVirtualSequencesCandidate(groups,mode);if(table.entries.empty())throw std::runtime_error("virtual bounds empty sequence table");result.sequences.clear();
    for(const auto&e:table.entries){IfmBoundsCandidate bounds{};std::memcpy(bounds.data(),groups.at(e.group).sequences.at(e.index).bounds.data(),24);checkIfmBoundsFiniteCandidate(bounds);result.sequences.push_back(bounds);}return result;
}
