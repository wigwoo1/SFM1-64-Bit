#include "source_bsp_inline_brush_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include "model_render_inputs_candidate.h"
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: test_source_bsp_world_materials map.bsp gameinfo.txt");
        const auto mapBytes = readBinaryAssetCandidate(argv[1]);
        const auto brush = readSourceBspInlineBrushCandidate(mapBytes, 0);
        const auto model = sourceBspStaticModelCandidate(brush);
        std::ifstream gameinfoInput(argv[2], std::ios::binary);
        if (!gameinfoInput) throw std::runtime_error("GameInfo open failed");
        const std::string gameinfo((std::istreambuf_iterator<char>(gameinfoInput)), {});
        const std::filesystem::path gameinfoPath(argv[2]);
        const auto mounts = gameinfoMountsCandidate(gameinfo, gameinfoPath.parent_path(), gameinfoPath.parent_path().parent_path());
        const auto inputs = prepareModelRenderInputsCandidate(model, 0, mounts, 0);
        size_t outOfRangeAlpha=0;for(float alpha:brush.displacementAlphas){if(!std::isfinite(alpha))throw std::runtime_error("nonfinite displacement alpha");if(alpha<0.f||alpha>255.f)++outOfRangeAlpha;}
        size_t transitionMaterials=0;for(const auto&material:inputs.materials)if(vmtAsciiLowerCandidate(material.source.syntax.key)=="worldvertextransition"){++transitionMaterials;if(!material.vertexTransition||material.source.baseTexture2.empty()||!material.secondBaseImage.width||!material.secondBaseImage.height||material.secondBaseImage.rgba.size()!=uint64_t(material.secondBaseImage.width)*material.secondBaseImage.height*4)throw std::runtime_error("WorldVertexTransition second-layer VTF or draw flag was not retained");}else if(material.vertexTransition)throw std::runtime_error("non-transition material flagged for vertex blending");
        if (inputs.materials.size() != 59 || inputs.indices.size() / 3 != 27147 || inputs.ranges.size() != 59 || model.geometry.vertices.size() != 42680 || brush.displacementFaces != 43 || brush.displacementVertices != 3371 || brush.displacementAlphas.size() != brush.positions.size() || outOfRangeAlpha != 55 || !transitionMaterials || brush.displacementTrianglesRemoved != 0)
            throw std::runtime_error("world BSP material draw-input regression");
        std::cout << "world_material_inputs_passed=1 materials=" << inputs.materials.size()
                  << " ranges=" << inputs.ranges.size() << " triangles=" << inputs.indices.size() / 3
                  << " mounts=" << mounts.size() << " world_vertices=" << model.geometry.vertices.size()
                  << " displacement_faces_tessellated=" << brush.displacementFaces
                  << " displacement_vertices=" << brush.displacementVertices
                  << " displacement_alpha_values=" << brush.displacementAlphas.size()
                  << " displacement_alpha_out_of_range=" << outOfRangeAlpha
                  << " world_vertex_transition_materials=" << transitionMaterials
                  << " displacement_triangles_removed=" << brush.displacementTrianglesRemoved
                  << " lightmaps=sampled_per_vertex\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
