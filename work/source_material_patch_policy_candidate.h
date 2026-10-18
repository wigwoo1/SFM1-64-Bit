#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
// Typed KeyValues state for material patch policy, not an effective shader or
// file loader. Children survive scalar writes in the original representation.
// 0=block,1=string,2=int32,3=float32,4=opaque pointer identity. Pointer IDs are
// owned64 semantic tokens; they are never dereferenced here. Other KV types
// and unbounded/non-ASCII input are explicitly outside this checked subset.
struct SourceMaterialPatchNodeCandidate {
 std::string key;uint32_t kind=0;std::string text;uint32_t bits=0;uint64_t pointer=0;
 std::vector<SourceMaterialPatchNodeCandidate>children;
};
inline std::string sourceMaterialPatchLowerCandidate(std::string s){for(char&c:s)if(c>='A'&&c<='Z')c=char(c+32);return s;}
inline void sourceMaterialPatchValidateCandidate(const SourceMaterialPatchNodeCandidate&n,size_t depth,size_t&count){
 if(depth>16||++count>65536||n.kind>4||n.key.size()>4096||n.text.size()>(1u<<20))throw std::runtime_error("material patch checked extent/type");
 size_t segment=0;for(unsigned char c:n.key){if(c<32||c>126)throw std::runtime_error("material patch key encoding");if(c=='/')segment=0;else if(++segment>255)throw std::runtime_error("material patch key path segment");}
 for(unsigned char c:n.text)if(!c||c>126)throw std::runtime_error("material patch string encoding");
 if(n.kind==3){float value;std::memcpy(&value,&n.bits,4);if(!std::isfinite(value))throw std::runtime_error("material patch float nonfinite");}
 for(const auto&child:n.children)sourceMaterialPatchValidateCandidate(child,depth+1,count);
}
inline SourceMaterialPatchNodeCandidate*sourceMaterialPatchFindCandidate(SourceMaterialPatchNodeCandidate&n,const std::string&key,bool create){
 if(key.empty())return &n;auto slash=key.find('/');auto part=key.substr(0,slash),lower=sourceMaterialPatchLowerCandidate(part);SourceMaterialPatchNodeCandidate*found=nullptr;
 for(auto&child:n.children)if(sourceMaterialPatchLowerCandidate(child.key)==lower){found=&child;break;}
 if(!found){if(!create)return nullptr;SourceMaterialPatchNodeCandidate child;child.key=part;n.children.push_back(std::move(child));n.kind=0;found=&n.children.back();}
 return slash==std::string::npos?found:sourceMaterialPatchFindCandidate(*found,key.substr(slash+1),create);
}
inline const SourceMaterialPatchNodeCandidate*sourceMaterialPatchFindConstCandidate(const SourceMaterialPatchNodeCandidate&n,const std::string&key){
 if(key.empty())return &n;auto slash=key.find('/');auto lower=sourceMaterialPatchLowerCandidate(key.substr(0,slash));
 for(const auto&child:n.children)if(sourceMaterialPatchLowerCandidate(child.key)==lower)return slash==std::string::npos?&child:sourceMaterialPatchFindConstCandidate(child,key.substr(slash+1));return nullptr;
}
inline void sourceMaterialPatchCopyCandidate(SourceMaterialPatchNodeCandidate&target,const SourceMaterialPatchNodeCandidate&source,bool onlyExisting,bool recurse){
 for(const auto&child:source.children){if(onlyExisting&&!sourceMaterialPatchFindCandidate(target,child.key,false))continue;
  auto*destination=sourceMaterialPatchFindCandidate(target,child.key,true);
  if(child.kind==0)sourceMaterialPatchCopyCandidate(*destination,child,onlyExisting,true);
  else{destination->kind=child.kind;destination->text=child.text;destination->bits=child.bits;destination->pointer=child.pointer;}
 }
 // Original10004EE9 adds a marker if a recursively handled target has
 // no children. Creating it also changes a scalar target back to block type.
 if(recurse&&target.children.empty()){auto*dummy=sourceMaterialPatchFindCandidate(target,"__vmtpatchdummy",true);dummy->kind=2;dummy->bits=1;}
 // Original10004F08 tests onlyExisting, then traverses TARGET true subkeys
 // and finds corresponding SOURCE subkeys (not the reverse direction).
 if(onlyExisting)for(auto&child:target.children)if(child.kind==0){const auto*from=sourceMaterialPatchFindConstCandidate(source,child.key);if(from&&from->kind==0)sourceMaterialPatchCopyCandidate(child,*from,onlyExisting,false);}
}
inline SourceMaterialPatchNodeCandidate sourceMaterialPatchPolicyCandidate(const SourceMaterialPatchNodeCandidate&base,const SourceMaterialPatchNodeCandidate&patch){
 size_t count=0;sourceMaterialPatchValidateCandidate(base,0,count);sourceMaterialPatchValidateCandidate(patch,0,count);auto result=base;auto patchCopy=patch;
 // Original10004F80: insert applies all supplied values (including overwrites),
 // then replace applies only keys already present after that insertion pass.
 const auto*insert=sourceMaterialPatchFindCandidate(patchCopy,"insert",false);const auto*replace=sourceMaterialPatchFindCandidate(patchCopy,"replace",false);
 if(insert)sourceMaterialPatchCopyCandidate(result,*insert,false,false);
 if(replace)sourceMaterialPatchCopyCandidate(result,*replace,true,false);
 count=0;sourceMaterialPatchValidateCandidate(result,0,count);return result;
}
