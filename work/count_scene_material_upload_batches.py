from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
old='size_t preparationTasks=0;'
assert old in s;s=s.replace(old,'size_t preparationTasks=0,materialUploadBatches=0;',1)
old='sceneBridge.recordMaterialUploads(command);'
s=s.replace(old,'if(materialCache.preparedCount())++materialUploadBatches;sceneBridge.recordMaterialUploads(command);',1)
old='else if(!iteration)sceneBridge.recordMaterialUploads(command);'
new='else if(!iteration){if(materialCache.preparedCount())++materialUploadBatches;sceneBridge.recordMaterialUploads(command);}'
assert old in s;s=s.replace(old,new,1)
old='<<" geometry_allocations=1 material_uploads=1 render_targets=1 gpu_wait='
new='<<" geometry_allocations=1 material_uploads="<<materialUploadBatches<<" render_targets=1 gpu_wait='
assert old in s;s=s.replace(old,new,1)
p.write_text(s)
