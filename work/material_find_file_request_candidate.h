#pragma once
#include "material_find_cache_key_candidate.h"
// Empty pathID represents the native null path-ID pointer for '{' names.
struct MaterialFindFileRequestCandidate{std::string key,file,pathID="GAME",mode="rb";};
// Original FindMaterial77 miss -> LoadVMT: already-normalized key gets a
// materials/ prefix, except a leading '{', then .vmt. Native MAX_PATH loading
// truncation is deliberately not asserted by this unbounded semantic request.
// This is an engine request string; do NOT pass it as an unrestricted host path.
inline bool materialFindFileRequestCandidate(std::string_view input,MaterialFindFileRequestCandidate&output){
 MaterialFindFileRequestCandidate plan;if(!materialFindCacheKeyCandidate(input,plan.key))return false;
 const bool procedural=!plan.key.empty()&&plan.key[0]=='{';
 plan.file=(procedural?std::string():"materials/")+plan.key+".vmt";
 if(procedural)plan.pathID.clear();
 output=std::move(plan);return true;
}
