#include "source_bsp_inline_brush_candidate.h"
#include <iostream>

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("usage: inspect_source_bsp_world_model map.bsp");
        const auto bytes = readBinaryAssetCandidate(argv[1]);
        const auto world = readSourceBspInlineBrushCandidate(bytes, 0);
        if (world.positions.size() != 42680 || world.triangles.size() != 27147 || world.materials.size() != 59 || world.skippedDegenerateTriangles != 1460 || world.displacementFaces != 43 || world.displacementTrianglesRemoved != 0)
            throw std::runtime_error("mtt_soldier world BSP geometry regression");
        if (world.normals.size() != world.positions.size() || world.uvs.size() != world.positions.size() || world.lightColors.size() != world.positions.size() || world.displacementAlphas.size() != world.positions.size() || world.displacementVertices != 3371)
            throw std::runtime_error("world BSP vertex channel count mismatch");
        for (const auto& position : world.positions)
            for (float value : position)
                if (!std::isfinite(value)) throw std::runtime_error("nonfinite world BSP position");
        for (const auto& normal : world.normals)
            for (float value : normal)
                if (!std::isfinite(value)) throw std::runtime_error("nonfinite world BSP normal");
        for (const auto& uv : world.uvs)
            for (float value : uv)
                if (!std::isfinite(value)) throw std::runtime_error("nonfinite world BSP UV");
        std::array<float,3> lightMin{INFINITY,INFINITY,INFINITY},lightMax{-INFINITY,-INFINITY,-INFINITY};
        for (const auto& light : world.lightColors) for(size_t c=0;c<3;++c){if(!std::isfinite(light[c])||light[c]<0)throw std::runtime_error("invalid world BSP light sample");lightMin[c]=std::min(lightMin[c],light[c]);lightMax[c]=std::max(lightMax[c],light[c]);}
        if(lightMin==lightMax)throw std::runtime_error("world BSP lightmaps unexpectedly constant");
        size_t outOfRangeAlpha=0;float alphaMin=INFINITY,alphaMax=-INFINITY;
        for(float alpha:world.displacementAlphas){if(!std::isfinite(alpha))throw std::runtime_error("nonfinite world BSP displacement alpha");alphaMin=std::min(alphaMin,alpha);alphaMax=std::max(alphaMax,alpha);if(alpha<0.f||alpha>255.f)++outOfRangeAlpha;}
        if(outOfRangeAlpha!=55)throw std::runtime_error("mtt_soldier displacement alpha raw-data regression");
        for (const auto& triangle : world.triangles) {
            if (triangle.material >= world.materials.size()) throw std::runtime_error("world BSP material index");
            for (auto vertex : triangle.vertices)
                if (vertex >= world.positions.size()) throw std::runtime_error("world BSP vertex index");
        }
        std::cout << "world_vertices=" << world.positions.size()
                  << " world_fan_triangles=" << world.triangles.size()
                  << " world_materials=" << world.materials.size()
                  << " skipped_degenerate_triangles=" << world.skippedDegenerateTriangles
                  << " displacement_faces_tessellated=" << world.displacementFaces
                  << " displacement_vertices=" << world.displacementVertices
                  << " displacement_triangles_removed=" << world.displacementTrianglesRemoved
                  << " displacement_alpha_raw_range=" << alphaMin << ':' << alphaMax
                  << " displacement_alpha_out_of_range=" << outOfRangeAlpha
                  << " light_samples=" << world.lightColors.size()
                  << " light_range=" << lightMin[0] << ':' << lightMax[0] << ',' << lightMin[1] << ':' << lightMax[1] << ',' << lightMin[2] << ':' << lightMax[2]
                  << " bounds_min=" << world.boundsMin[0] << ',' << world.boundsMin[1] << ',' << world.boundsMin[2]
                  << " bounds_max=" << world.boundsMax[0] << ',' << world.boundsMax[1] << ',' << world.boundsMax[2]
                  << "\n";
        std::cout << "world_geometry_valid=1 lightmaps_sampled=1 displacements_tessellated=1\n";
        return world.triangles.empty() ? 1 : 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
