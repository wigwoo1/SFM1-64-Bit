#pragma once
#include "material_find_cache_key_candidate.h"
// Empty pathID represents the native null path-ID pointer for '{' names.
struct MaterialFindFileRequestCandidate{std::string key,file,pathID="GAME",mode="rb";};
// Original FindMaterial77 miss -> LoadVMT: already-normalized key gets a
// materials/ prefix, except a leading '{', then .vmt. Only requests that fit
// the original 260-byte buffer WITH a terminator are admitted. Longer original
// requests truncate; an exact 260-byte result is not reliably NUL-terminated.
// Refuse those inputs rather than emulate truncation or an out-of-bounds read.
// This is an engine request string; do NOT pass it as an unrestricted host path.
inline bool materialFindFileRequestCandidate(std::string_view input,MaterialFindFileRequestCandidate&output){
 MaterialFindFileRequestCandidate plan;if(!materialFindCacheKeyCandidate(input,plan.key))return false;
 const bool procedural=!plan.key.empty()&&plan.key[0]=='{';
 plan.file=(procedural?std::string():"materials/")+plan.key+".vmt";
 if(plan.file.size()>=260)return false;
 if(procedural)plan.pathID.clear();
 output=std::move(plan);return true;
}
