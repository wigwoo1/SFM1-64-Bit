from pathlib import Path
p=Path('work/test_scene_material_inputs.cpp');s=p.read_text().replace('#include "scene_material_draw_inputs_candidate.h"','#include "scene_material_draw_inputs_candidate.h"\n#include "prepared_session_scene_candidate.h"')
s=s.replace('float previous=2;','float previous=2;GeometryDeformationWorkerPoolCandidate workers(4);')
old='auto inputs=readSceneMaterialInputsCandidate(timeline.graph(),step.scene.models,cache,mounts);'
new='auto prepared=prepareSessionSceneCandidate(timeline.graph(),step.scene,asset,cache,workers,mounts);const auto&inputs=prepared.materialInputs;if(prepared.upload.vertices.size()!=47850||prepared.materialAlpha.size()!=prepared.upload.materials.size())throw std::runtime_error("prepared scene coverage");for(const auto&draw:prepared.upload.translucent){const auto&address=prepared.upload.materials.at(draw.materialSlot);if(address.model!=draw.model||address.material!=draw.materialIndex)throw std::runtime_error("prepared scene draw material");}std::cout<<"packed_scene vertices="<<prepared.upload.vertices.size()<<" indices="<<prepared.upload.indices.size()<<" opaque_ranges="<<prepared.upload.opaque.size()<<" translucent_ranges="<<prepared.upload.translucent.size()<<"\\n";'
if s.count(old)!=1:raise RuntimeError('scene inputs marker')
s=s.replace(old,new).replace('per_draw_alpha=1','joined_pose_geometry_materials=1 packed_alpha_slots=1 per_draw_alpha=1')
p.write_text(s)
