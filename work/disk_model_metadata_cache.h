#pragma once
#include "mdl_flex_metadata.h"
#include "mdl_flex_metadata_stream.h"
#include <filesystem>
#include <fstream>
#include <memory>
#include <map>
// Reconstruction adapter: fixed handles retained for cache lifetime; no reuse.
class DiskModelMetadataCacheCandidate {
 struct Record{std::filesystem::path path;uint32_t references=0;std::vector<MdlFlexControllerMetadata> controllers;};
 std::vector<std::unique_ptr<Record>> records_;std::map<std::filesystem::path,uint16_t> handles_;
public:
 uint16_t acquire(const std::filesystem::path&path){std::error_code canonicalError;auto key=std::filesystem::weakly_canonical(path,canonicalError);if(canonicalError){canonicalError.clear();key=std::filesystem::absolute(path,canonicalError).lexically_normal();if(canonicalError)throw std::filesystem::filesystem_error("model cache path",path,canonicalError);}auto found=handles_.find(key);if(found!=handles_.end()){auto&r=*records_.at(found->second);if(r.references==UINT32_MAX)throw std::runtime_error("model reference overflow");++r.references;return found->second;}
 std::ifstream f(key,std::ios::binary);if(!f)return 0xffff;auto controllers=readMdl49FlexMetadataStream(f);if(records_.size()>=0xffff)throw std::runtime_error("model cache capacity");auto record=std::make_unique<Record>();record->path=key;record->references=1;record->controllers=std::move(controllers);auto handle=static_cast<uint16_t>(records_.size());records_.push_back(std::move(record));try{handles_.emplace(key,handle);}catch(...){records_.pop_back();throw;}return handle;}
 void release(uint16_t handle){auto&r=*records_.at(handle);if(!r.references)throw std::runtime_error("model reference underflow");--r.references;}
 std::vector<MdlFlexControllerMetadata>* controllers(uint16_t handle){if(handle==0xffff)return nullptr;auto&r=*records_.at(handle);return r.references?&r.controllers:nullptr;}
 uint32_t references(uint16_t handle)const{return records_.at(handle)->references;}
};
