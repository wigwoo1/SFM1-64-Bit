#pragma once
#include "generic_ambient_irradiance_candidate.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <optional>
#include <vector>

// Value-owned GPU input, separate from SceneGpuVertexCandidate::normal, which
// carries a light color for BSP-world vertices. Only pre-world model geometry
// (SceneGeometryModelCandidate) is eligible for this preparation function.
// Explicit cubes are already selected by the caller: no implicit defaults,
// model-origin choice, BSP lookup, HDR choice or material-path inference.
struct SceneModelAmbientRecordCandidate {
    std::array<float,4>worldNormal{};
    std::array<std::array<float,4>,6>cube{};
};
static_assert(sizeof(SceneModelAmbientRecordCandidate)==112,"ambient record ABI");
struct SceneModelAmbientAddressCandidate {size_t model,vertex;};
struct SceneModelAmbientRecordsCandidate {
    std::vector<SceneModelAmbientRecordCandidate>records;
    std::vector<SceneModelAmbientAddressCandidate>addresses;
};
inline std::array<float,3> sceneAmbientNormalReferenceCandidate(const std::array<float,3>&normal){
    for(float x:normal)if(!std::isfinite(x))throw std::invalid_argument("nonfinite model ambient normal");
    const float xx=normal[0]*normal[0],yy=normal[1]*normal[1],zz=normal[2]*normal[2];
    const float xy=xx+yy,lengthSquared=xy+zz;
    if(!(lengthSquared>0)||!std::isfinite(lengthSquared))throw std::invalid_argument("model ambient normal length");
    const float inverse=1.0f/std::sqrt(lengthSquared);
    return {normal[0]*inverse,normal[1]*inverse,normal[2]*inverse};
}
template<class Models>
SceneModelAmbientRecordsCandidate prepareSceneModelAmbientRecordsCandidate(
    const Models&models,const std::vector<std::optional<GenericAmbientCubeCandidate>>&cubes){
    if(cubes.size()!=models.size())throw std::invalid_argument("model ambient cube coverage");
    SceneModelAmbientRecordsCandidate result;
    for(size_t model=0;model<models.size();++model){
        // Missing and invisible model inputs remain unused, including normals.
        if(!models[model].model.visible||!cubes[model])continue;
        const auto&cube=*cubes[model];
        for(const auto&face:cube)for(float x:face)if(!std::isfinite(x))throw std::invalid_argument("nonfinite model ambient cube");
        for(size_t vertex=0;vertex<models[model].vertices.size();++vertex){
            const auto&normal=models[model].vertices[vertex].normal;
            (void)sceneAmbientNormalReferenceCandidate(normal); // owned finite/nonzero domain
            SceneModelAmbientRecordCandidate record;
            std::copy(normal.begin(),normal.end(),record.worldNormal.begin());
            for(unsigned face=0;face<6;++face)std::copy(cube[face].begin(),cube[face].end(),record.cube[face].begin());
            result.records.push_back(record);result.addresses.push_back({model,vertex});
        }
    }
    return result; // no input mutation, persistent pointers or partial output commit
}
inline std::array<float,3> sceneModelAmbientRecordReferenceCandidate(const SceneModelAmbientRecordCandidate&record){
    GenericAmbientCubeCandidate cube{};
    for(unsigned face=0;face<6;++face)std::copy_n(record.cube[face].begin(),3,cube[face].begin());
    return genericAmbientIrradianceCandidate(sceneAmbientNormalReferenceCandidate(
        {record.worldNormal[0],record.worldNormal[1],record.worldNormal[2]}),cube);
}
