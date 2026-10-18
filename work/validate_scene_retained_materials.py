from pathlib import Path
p=Path('work/test_scene_geometry_preparation.cpp')
s=p.read_text().replace('#include "scene_gpu_upload_candidate.h"','#include "scene_gpu_upload_candidate.h"\n#include "scene_render_inputs_cache_candidate.h"\n#include "gameinfo_mounts_candidate.h"')
needle='GeometryDeformationWorkerPoolCandidate workers(4);'
replacement='''std::ifstream gameinfo(content/"gameinfo.txt",std::ios::binary);if(!gameinfo)throw std::runtime_error("scene GameInfo missing");std::string gameinfoText((std::istreambuf_iterator<char>(gameinfo)),{});SceneRenderInputsCacheCandidate materialCache(gameinfoMountsCandidate(gameinfoText,content,content.parent_path()),2,asset);size_t materialFailures=0;const ModelRenderInputsCandidate*retainedMaterial=nullptr;GeometryDeformationWorkerPoolCandidate workers(4);'''
if s.count(needle)!=1:raise RuntimeError('workers marker')
s=s.replace(needle,replacement)
needle='if(geometry.size()!=3)throw std::runtime_error("scene geometry model count");'
replacement=needle+'''for(const auto&instance:geometry){try{const auto&input=materialCache.get(instance.model.placement.asset,instance.model.skin,instance.model.body);if(instance.model.id!=34561)throw std::runtime_error("unexpected resolved scene materials");if(retainedMaterial&&retainedMaterial!=&input)throw std::runtime_error("material cache reference changed");retainedMaterial=&input;}catch(const std::runtime_error&e){if(instance.model.id==34561)throw;const std::string message=e.what();if(message.find("texture")==std::string::npos)throw;std::cout<<"material_pending model="<<instance.model.id<<" reason="<<message<<"\\n";++materialFailures;}}'''
if s.count(needle)!=1:raise RuntimeError('geometry marker')
s=s.replace(needle,replacement)
needle='std::cout<<"scene CPU geometry'
s=s.replace(needle,'if(materialCache.size()!=1||materialCache.hits()!=2||materialFailures!=6)throw std::runtime_error("retained material coverage");std::cout<<"retained_material_entries="<<materialCache.size()<<" cache_hits="<<materialCache.hits()<<" unresolved_texture_attempts="<<materialFailures<<"\\n";'+needle)
p.write_text(s)
