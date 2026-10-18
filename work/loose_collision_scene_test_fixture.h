#pragma once
#include "source_loose_collision_scene_assets_candidate.h"
#include "source_static_prop_point_spatial_candidate.h"
inline std::vector<std::filesystem::path>looseSceneFixtureMounts(const char*path){std::ifstream in(path);if(!in)throw std::runtime_error("scene fixture mounts read");std::vector<std::filesystem::path>out;std::string line;while(std::getline(in,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(!line.empty())out.emplace_back(line);}return out;}
inline std::vector<uint64_t>looseSceneFixtureIds(size_t count,uint32_t low){std::vector<uint64_t>out;for(size_t i=0;i<count;++i)out.push_back((uint64_t(i+1)<<32)|low);return out;}
struct LooseCollisionSceneTestFixture {
    SourceBspStaticPropsCandidate props;
    std::vector<std::filesystem::path>mounts;
    std::vector<uint64_t>modelIds,entityIds;
    SourceLooseCollisionSceneAssetsCandidate assets;
    SourceStaticPropPointSpatialCandidate scene;
    LooseCollisionSceneTestFixture(const char*bsp,const char*mountList):props(readBinaryAssetCandidate(bsp)),mounts(looseSceneFixtureMounts(mountList)),modelIds(looseSceneFixtureIds(props.models().size(),0x12345678)),entityIds(looseSceneFixtureIds(props.records().size(),0xdecafbad)),assets(props,mounts,modelIds),scene(props,assets.models(),entityIds){}
};
