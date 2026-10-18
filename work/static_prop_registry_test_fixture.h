#pragma once
#include "source_static_prop_point_registry_candidate.h"
#include <fstream>
#include <iterator>
#include <sstream>
inline uint64_t staticPropEntityFixtureId(size_t i){return (uint64_t(i+1)<<32)|0xdecafbadu;}
inline uint64_t staticPropModelFixtureId(size_t i){return (uint64_t(i+1)<<32)|0x12345678u;}
inline uint16_t staticPropSurfaceFixtureId(size_t i){return uint16_t(i*1009u);}
struct StaticPropRegistryTestFixture {
    SourceBspStaticPropsCandidate props;
    std::vector<SourceStaticPropPointModelCandidate>models;
    std::vector<uint64_t>ids;
    std::unique_ptr<SourceStaticPropPointRegistryCandidate>registry;
    StaticPropRegistryTestFixture(const char*bsp,const char*mountList):props(readBinaryAssetCandidate(bsp)){
        std::ifstream f(mountList);if(!f)throw std::runtime_error("registry fixture mounts read");std::string line;std::vector<std::filesystem::path>mounts;while(std::getline(f,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(!line.empty())mounts.emplace_back(line);}
        for(size_t i=0;i<props.models().size();++i){const auto&name=props.models()[i];std::filesystem::path mdl;for(const auto&mount:mounts){const auto path=mount/name;if(std::filesystem::is_regular_file(path)){mdl=path;break;}}if(mdl.empty())throw std::runtime_error("registry fixture model unresolved");const auto bytes=readBinaryAssetCandidate(mdl);const auto read=sourceStudioReadLooseCollisionCandidate(name,mounts);auto owner=std::make_shared<const SourceStudioLooseCollisionAssetCandidate>(bytes,read);models.push_back({name,staticPropModelFixtureId(i),std::move(owner),staticPropSurfaceFixtureId(i)});}
        for(size_t i=0;i<props.records().size();++i)ids.push_back(staticPropEntityFixtureId(i));registry=std::make_unique<SourceStaticPropPointRegistryCandidate>(props,models,ids);
    }
};
