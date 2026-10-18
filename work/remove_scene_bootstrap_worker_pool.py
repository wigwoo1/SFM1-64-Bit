from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
old='GeometryDeformationWorkerPoolCandidate deformationPool(geometryWorkers);'
new='std::unique_ptr<GeometryDeformationWorkerPoolCandidate>deformationPool;if(!sceneRequested)deformationPool=std::make_unique<GeometryDeformationWorkerPoolCandidate>(geometryWorkers);else std::cout<<"scene_bootstrap_geometry_workers=0\\n";'
assert old in s;s=s.replace(old,new,1)
old='auto deformed=deformationPool.deform(model.geometry,palette);'
new='auto deformed=sceneRequested?std::vector<GeometryDeformedVertexCandidate>{}:deformationPool->deform(model.geometry,palette);'
assert old in s;s=s.replace(old,new,1)
s=s.replace('deformationPool.deform(model.geometry,framePalette)','deformationPool->deform(model.geometry,framePalette)')
s=s.replace('deformationPool.workerCount()','(sceneRequested?geometryWorkers:deformationPool->workerCount())')
p.write_text(s)
