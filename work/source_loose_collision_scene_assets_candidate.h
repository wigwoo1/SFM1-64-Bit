#pragma once
#include "source_static_prop_point_registry_candidate.h"
#include "source_surface_name_table_candidate.h"
#include "source_studio_surface_properties_candidate.h"
#include "vmt_keyvalues_candidate.h"
#include <unordered_set>

// Explicit ordered loose-directory provider, not live SFM/Steam/archive lookup.
// Complete construction before publishing its models into a spatial snapshot.
// Surface data here are NAME/INDEX only, never physical/audio material values.
class SourceLooseCollisionSceneAssetsCandidate {
    struct Prepared {
        std::filesystem::path manifest;
        std::vector<std::filesystem::path>scripts,models;
        std::vector<SourceStaticPropPointModelCandidate>bindings;
        std::vector<SourceStudioSurfacePropertiesCandidate>surfaces;
        std::unique_ptr<const SourceSurfaceNameTableCandidate>table;
    } data_;
    static std::filesystem::path relative(std::string name,const std::string&prefix,const std::string&extension){
        for(char&c:name)if(c=='\\')c='/';
        if(name.empty()||name.size()>=260)throw std::runtime_error("loose collision scene asset path extent");
        std::string part;
        for(unsigned char c:name){if(c<32||c>126||c==':'||c=='|'||c=='*'||c=='?'||c=='"'||c=='<'||c=='>')throw std::runtime_error("unsafe loose collision scene asset path");if(c=='/'){if(part.empty()||part=="."||part=="..")throw std::runtime_error("unsafe loose collision scene path segment");part.clear();}else part+=char(c);}
        const auto lower=vmtAsciiLowerCandidate(name);if(part.empty()||part=="."||part==".."||lower.size()<prefix.size()+extension.size()+1||lower.compare(0,prefix.size(),prefix)||lower.compare(lower.size()-extension.size(),extension.size(),extension))throw std::runtime_error("unsupported loose collision scene asset path");
        return std::filesystem::path(name);
    }
    static void validateMounts(const std::vector<std::filesystem::path>&mounts){
        if(mounts.empty())throw std::runtime_error("loose collision scene mount scope empty");
        for(const auto&mount:mounts){
            if(!mount.is_absolute())throw std::runtime_error("absolute loose collision scene mount required");
            std::error_code ec;const auto state=std::filesystem::status(mount,ec);if(ec){if(sourceStudioCollisionMissingErrorCandidate(ec))continue;throw std::system_error(ec,"loose collision scene mount status");}if(!std::filesystem::exists(state))continue;if(!std::filesystem::is_directory(state))throw std::runtime_error("loose collision scene mount is not a directory");
            for(const auto&e:std::filesystem::directory_iterator(mount))if(vmtAsciiLowerCandidate(e.path().extension().string())==".vpk")throw std::runtime_error("loose collision scene VPK provider not implemented");
        }
    }
    struct File {std::filesystem::path path;std::vector<uint8_t>bytes;};
    static File read(const std::filesystem::path&name,const std::vector<std::filesystem::path>&mounts){
        for(const auto&mount:mounts){const auto path=mount/name;std::error_code ec;const auto state=std::filesystem::status(path,ec);if(ec){if(sourceStudioCollisionMissingErrorCandidate(ec))continue;throw std::system_error(ec,"loose collision scene file status");}if(!std::filesystem::exists(state))continue;if(!std::filesystem::is_regular_file(state))throw std::runtime_error("loose collision scene source is not a regular file");return {path,readBinaryAssetCandidate(path)};}
        throw std::runtime_error("required loose collision scene asset missing: "+name.string());
    }
    static std::string text(const File&file){for(unsigned char c:file.bytes)if((c<32&&c!='\t'&&c!='\r'&&c!='\n')||c>126)throw std::runtime_error("unsupported loose collision scene script encoding");return std::string(file.bytes.begin(),file.bytes.end());}
    static Prepared prepare(const SourceBspStaticPropsCandidate&props,
        const std::vector<std::filesystem::path>&mounts,const std::vector<uint64_t>&modelIds){
        if(modelIds.size()!=props.models().size())throw std::runtime_error("loose collision scene model identity count");std::unordered_set<uint64_t>seen;for(uint64_t id:modelIds)if(!id||!seen.insert(id).second)throw std::runtime_error("loose collision scene model identity duplicate/zero");validateMounts(mounts);
        Prepared out;const auto manifest=read(relative("scripts/surfaceproperties_manifest.txt","scripts/",".txt"),mounts);out.manifest=manifest.path;
        const auto root=parseVmtCandidate(text(manifest));if(vmtAsciiLowerCandidate(root.key)!="surfaceproperties_manifest")throw std::runtime_error("unsupported surface manifest root");
        std::vector<SourceSurfaceScriptCandidate>scripts;
        for(const auto&entry:root.children){if(entry.block||vmtAsciiLowerCandidate(entry.key)!="file")throw std::runtime_error("unsupported surface manifest entry");const auto path=relative(entry.value,"scripts/",".txt");const auto file=read(path,mounts);scripts.push_back({path.generic_string(),text(file)});out.scripts.push_back(file.path);}
        if(scripts.empty())throw std::runtime_error("surface manifest contains no scripts");out.table=std::make_unique<const SourceSurfaceNameTableCandidate>(scripts);
        // Original parser requires default at index0. Reject incomplete startup
        // input rather than quietly publishing an unusable name namespace.
        if(out.table->lookup("default")!=0)throw std::runtime_error("surface manifest default property must have index zero");
        for(size_t i=0;i<props.models().size();++i){const auto&name=props.models()[i];const auto model=read(relative(name,"models/",".mdl"),mounts);auto surfaces=sourceStudioResolveSurfacePropertiesCandidate(model.bytes,*out.table);auto collision=sourceStudioReadLooseCollisionCandidate(name,mounts);auto asset=std::make_shared<const SourceStudioLooseCollisionAssetCandidate>(model.bytes,collision);out.bindings.push_back({name,modelIds[i],std::move(asset),surfaces.traceSurfaceProperty()});out.surfaces.push_back(std::move(surfaces));out.models.push_back(model.path);}
        return out;
    }
public:
    SourceLooseCollisionSceneAssetsCandidate(const SourceBspStaticPropsCandidate&props,
        const std::vector<std::filesystem::path>&mounts,const std::vector<uint64_t>&modelIds)
        :data_(prepare(props,mounts,modelIds)){}
    const auto&models()const{return data_.bindings;}
    const auto&modelSurfaces()const{return data_.surfaces;}
    const auto&surfaceNames()const{return *data_.table;}
    const auto&manifestSource()const{return data_.manifest;}
    const auto&scriptSources()const{return data_.scripts;}
    const auto&modelSources()const{return data_.models;}
};
