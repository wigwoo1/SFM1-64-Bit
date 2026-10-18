#pragma once
#include "read_binary_asset_candidate.h"
#include "source_phy_point_trace_candidate.h"
#include "source_studio_physics_contents_candidate.h"
#include <memory>
#include <string>
#include <system_error>
// Explicit provider scope, never a claim about live SFM/Steam/dynamic mounts.
enum class SourceStudioCollisionReadKindCandidate {Unresolved,MissingInConfiguredLooseMounts,Bytes};
struct SourceStudioCollisionReadCandidate {
    SourceStudioCollisionReadKindCandidate kind=SourceStudioCollisionReadKindCandidate::Unresolved;
    std::filesystem::path source;
    std::vector<uint8_t>bytes;
};
inline std::filesystem::path sourceStudioCollisionRelativePathCandidate(std::string name){
    for(char&c:name)if(c=='\\')c='/';
    if(name.empty()||name.size()>=260||name.front()=='/'||name.back()=='/')throw std::runtime_error("invalid collision model path");
    std::string part,lower=name;for(char&c:lower)if(c>='A'&&c<='Z')c=char(c+32);
    for(unsigned char c:name){if(c<32||c>126||c==':'||c=='|'||c=='*'||c=='?'||c=='"'||c=='<'||c=='>')throw std::runtime_error("unsafe collision model path");if(c=='/'){if(part.empty()||part=="."||part=="..")throw std::runtime_error("unsafe collision model segment");part.clear();}else part+=char(c);}
    if(part.empty()||part=="."||part==".."||lower.size()<11||lower.compare(0,7,"models/")||lower.compare(lower.size()-4,4,".mdl"))throw std::runtime_error("unsupported collision model path");
    auto path=std::filesystem::path(name);path.replace_extension(".phy");return path;
}
inline bool sourceStudioCollisionMissingErrorCandidate(const std::error_code&e){return e==std::errc::no_such_file_or_directory||e==std::errc::not_a_directory;}
// Current explicit loose-directory backend. Reject archives rather than
// treating unimplemented providers as evidence of a missing collision file.
inline SourceStudioCollisionReadCandidate sourceStudioReadLooseCollisionCandidate(
    const std::string&model,const std::vector<std::filesystem::path>&mounts){
    const auto relative=sourceStudioCollisionRelativePathCandidate(model);if(mounts.empty())throw std::runtime_error("collision mount scope empty");
    for(const auto&mount:mounts){if(!mount.is_absolute())throw std::runtime_error("absolute collision mount required");std::error_code ec;const auto status=std::filesystem::status(mount,ec);if(ec){if(sourceStudioCollisionMissingErrorCandidate(ec))continue;throw std::system_error(ec,"collision mount status");}if(!std::filesystem::exists(status))continue;if(!std::filesystem::is_directory(status))throw std::runtime_error("collision mount is not a directory");for(const auto&entry:std::filesystem::directory_iterator(mount)){auto extension=entry.path().extension().string();for(char&c:extension)if(c>='A'&&c<='Z')c=char(c+32);if(extension==".vpk")throw std::runtime_error("collision VPK provider not implemented");}}
    for(const auto&mount:mounts){const auto path=mount/relative;std::error_code ec;const auto status=std::filesystem::status(path,ec);if(ec){if(sourceStudioCollisionMissingErrorCandidate(ec))continue;throw std::system_error(ec,"collision file status");}if(!std::filesystem::exists(status))continue;if(!std::filesystem::is_regular_file(status))throw std::runtime_error("collision source is not a regular file");return {SourceStudioCollisionReadKindCandidate::Bytes,path,readBinaryAssetCandidate(path)};}
    return {SourceStudioCollisionReadKindCandidate::MissingInConfiguredLooseMounts,{},{}};
}
// Completed synchronous NO-INCLUDED-MODEL / one-solid point-collision owner.
// Constructors publish no partial object on IO/parse failure. Shared immutable
// shape ownership outlives temporary file buffers and supports cache retention.
// Asynchronous scheduling, included-model discovery, maps' embedded archives,
// material properties and live scene registry integration remain separate.
class SourceStudioLooseCollisionAssetCandidate {
    SourceStudioPhysicsContentsCandidate studio_;
    std::shared_ptr<const SourcePhyPointTraceCandidate>shape_;
    std::filesystem::path source_;
    bool missing_=false;
public:
    SourceStudioLooseCollisionAssetCandidate(const std::vector<uint8_t>&mdl,const SourceStudioCollisionReadCandidate&read):studio_(mdl){
        uint32_t includes=0;std::memcpy(&includes,mdl.data()+336,4);if(includes)throw std::runtime_error("collision included-model resolution required");
        switch(read.kind){
        case SourceStudioCollisionReadKindCandidate::Unresolved:throw std::runtime_error("collision read unresolved");
        case SourceStudioCollisionReadKindCandidate::MissingInConfiguredLooseMounts:if(!read.bytes.empty()||!read.source.empty())throw std::runtime_error("inconsistent missing collision read");missing_=true;break;
        case SourceStudioCollisionReadKindCandidate::Bytes:if(read.source.empty())throw std::runtime_error("collision source identity required");shape_=std::make_shared<const SourcePhyPointTraceCandidate>(read.bytes);source_=read.source;break;
        default:throw std::runtime_error("invalid collision read kind");
        }
    }
    bool missingInConfiguredLooseMounts()const{return missing_;}
    const auto&studio()const{return studio_;}
    const auto&sharedShape()const{return shape_;}
    const auto&source()const{return source_;}
};
