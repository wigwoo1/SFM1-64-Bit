#pragma once
#include "retained_obj_scene_candidate.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <filesystem>
#include <cstring>
#include <fstream>
// Versioned little-endian snapshot embeds original geometry and decoded images.
inline constexpr size_t objSnapshotLimitCandidate=256u*1024u*1024u;
struct ObjSnapshotWriterCandidate{
 std::vector<uint8_t> bytes;
 void raw(const void*p,size_t n){if(n>objSnapshotLimitCandidate-bytes.size())throw std::runtime_error("OBJ snapshot too large");auto first=static_cast<const uint8_t*>(p);bytes.insert(bytes.end(),first,first+n);}
 void integer(uint32_t n){uint8_t b[4]{uint8_t(n),uint8_t(n>>8),uint8_t(n>>16),uint8_t(n>>24)};raw(b,4);}
 void count(size_t n){if(n>UINT32_MAX)throw std::runtime_error("OBJ snapshot count overflow");integer(uint32_t(n));}
 void number(float f){uint32_t n;static_assert(sizeof(f)==sizeof(n));std::memcpy(&n,&f,4);integer(n);}
 void text(const std::string&s){count(s.size());raw(s.data(),s.size());}
};
struct ObjSnapshotReaderCandidate{
 const std::vector<uint8_t>&bytes;size_t at=0;
 void raw(void*p,size_t n){if(n>bytes.size()-at)throw std::runtime_error("Truncated OBJ snapshot");std::memcpy(p,bytes.data()+at,n);at+=n;}
 uint32_t integer(){uint8_t b[4];raw(b,4);return uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
 size_t count(size_t minimum=1){auto n=integer();if(n>(bytes.size()-at)/minimum)throw std::runtime_error("OBJ snapshot count exceeds remaining bytes");return n;}
 float number(){auto n=integer();float f;std::memcpy(&f,&n,4);if(!std::isfinite(f))throw std::runtime_error("Nonfinite OBJ snapshot value");return f;}
 std::string text(){auto n=count();if(n>1024*1024)throw std::runtime_error("OBJ snapshot string too long");std::string s(n,'\0');raw(s.data(),n);if(s.find('\0')!=std::string::npos)throw std::runtime_error("OBJ snapshot string contains null");return s;}
};
inline std::vector<uint8_t> studioEncodeObjSnapshotCandidate(const RetainedObjSceneCandidate&owner){
 ObjSnapshotWriterCandidate w;w.raw("SFM64OBJ",8);w.integer(1);w.number(owner.scale());const auto&m=owner.sourceModel();
 for(const auto*list:{&m.positions,&m.textures,&m.normals}){w.count(list->size());for(const auto&v:*list)for(float f:v)w.number(f);}
 w.count(m.polygons.size());for(const auto&p:m.polygons){w.text(p.object);w.text(p.group);w.text(p.material);w.text(p.smoothing);w.count(p.corners.size());for(const auto&c:p.corners){w.integer(c.position);w.integer(c.texture);w.integer(c.normal);}}
 w.count(m.materialLibraries.size());for(const auto&s:m.materialLibraries)w.text(s);
 const auto&names=owner.mesh().materials;w.count(names.size());for(size_t i=0;i<names.size();++i){w.text(names[i]);const auto&image=owner.textureInputs().materials.at(i).image;w.integer(image.width);w.integer(image.height);w.count(image.rgba.size());w.raw(image.rgba.data(),image.rgba.size());}return std::move(w.bytes);
}
inline std::shared_ptr<const RetainedObjSceneCandidate> studioDecodeObjSnapshotCandidate(const std::vector<uint8_t>&bytes){
 if(bytes.size()>objSnapshotLimitCandidate)throw std::runtime_error("OBJ snapshot too large");ObjSnapshotReaderCandidate r{bytes};char magic[8];r.raw(magic,8);if(std::memcmp(magic,"SFM64OBJ",8)||r.integer()!=1)throw std::runtime_error("Unsupported OBJ snapshot format");auto scale=r.number();StudioObjModelCandidate m;
 for(auto*list:{&m.positions,&m.textures,&m.normals}){auto n=r.count(12);list->resize(n);for(auto&v:*list)for(float&f:v)f=r.number();}
 auto polygons=r.count(20);if(polygons>1000000)throw std::runtime_error("OBJ snapshot polygon limit");m.polygons.resize(polygons);for(auto&p:m.polygons){p.object=r.text();p.group=r.text();p.material=r.text();p.smoothing=r.text();auto corners=r.count(12);if(corners<3)throw std::runtime_error("OBJ snapshot polygon too small");p.corners.resize(corners);for(auto&c:p.corners){c.position=r.integer();c.texture=r.integer();c.normal=r.integer();if(c.position>=m.positions.size()||(c.texture!=UINT32_MAX&&c.texture>=m.textures.size())||(c.normal!=UINT32_MAX&&c.normal>=m.normals.size()))throw std::runtime_error("OBJ snapshot corner index invalid");}}
 auto libraries=r.count(4);for(size_t i=0;i<libraries;++i)m.materialLibraries.push_back(r.text());std::map<std::string,VtfRgbaImageCandidate> textures;auto count=r.count(16);for(size_t i=0;i<count;++i){auto name=r.text();VtfRgbaImageCandidate image;image.width=r.integer();image.height=r.integer();auto n=r.count();if(!image.width||!image.height||image.width>16384||image.height>16384||uint64_t(image.width)*image.height*4!=n)throw std::runtime_error("OBJ snapshot image dimensions invalid");image.rgba.resize(n);r.raw(image.rgba.data(),n);if(!textures.emplace(name,std::move(image)).second)throw std::runtime_error("Duplicate OBJ snapshot material");}
 if(r.at!=bytes.size()||m.polygons.empty())throw std::runtime_error("OBJ snapshot trailing bytes or empty mesh");auto owner=std::make_shared<RetainedObjSceneCandidate>(m,std::move(textures),scale);if(owner->mesh().materials.size()!=count)throw std::runtime_error("OBJ snapshot missing material images");return owner;
}
inline std::shared_ptr<const RetainedObjSceneCandidate> studioLoadObjSnapshotCandidate(const std::filesystem::path&path){std::ifstream file(path,std::ios::binary|std::ios::ate);if(!file)throw std::runtime_error("Cannot open OBJ snapshot");auto size=file.tellg();if(size<0||uint64_t(size)>objSnapshotLimitCandidate)throw std::runtime_error("OBJ snapshot file size invalid");std::vector<uint8_t> bytes(size_t(size),0);file.seekg(0);if(!file.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size())))throw std::runtime_error("Cannot read OBJ snapshot");return studioDecodeObjSnapshotCandidate(bytes);}
inline void studioSaveObjSnapshotCandidate(const RetainedObjSceneCandidate&owner,const std::filesystem::path&path){auto bytes=studioEncodeObjSnapshotCandidate(owner);auto checked=studioDecodeObjSnapshotCandidate(bytes);(void)checked;auto temporary=path;temporary+=L".tmp-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());try{std::ofstream file(temporary,std::ios::binary|std::ios::trunc);if(!file||!file.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size())))throw std::runtime_error("Cannot write OBJ snapshot");file.close();if(!file)throw std::runtime_error("Cannot close OBJ snapshot");if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot publish OBJ snapshot");}catch(...){std::error_code ignored;std::filesystem::remove(temporary,ignored);throw;}}
