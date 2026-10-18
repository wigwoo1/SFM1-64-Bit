from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
old='size_t vertexCapacity=vertices.size()*sizeof(Vertex),indexCapacity=indices.size()*sizeof(uint32_t);'
new='size_t vertexCapacity=std::max(sizeof(Vertex),vertices.size()*sizeof(Vertex)),indexCapacity=std::max(sizeof(uint32_t),indices.size()*sizeof(uint32_t));'
assert old in s;s=s.replace(old,new,1)
old='buffer(vertices.size()*sizeof(Vertex),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,vertices.data(),vertexMemory)'
new='buffer(vertexCapacity,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,vertices.empty()?nullptr:vertices.data(),vertexMemory)'
assert old in s;s=s.replace(old,new,1)
old='buffer(indices.size()*4,VK_BUFFER_USAGE_INDEX_BUFFER_BIT,indices.data(),indexMemory)'
new='buffer(indexCapacity,VK_BUFFER_USAGE_INDEX_BUFFER_BIT,indices.empty()?nullptr:indices.data(),indexMemory)'
assert old in s;s=s.replace(old,new,1)
old='bool modelVisible=true;size_t renderedFrameCount=0;'
new='bool modelVisible=!frozenScene||!frozenScene->upload.opaque.empty()||!frozenScene->upload.translucent.empty();size_t renderedFrameCount=0;'
assert old in s;s=s.replace(old,new,1)
old='camera=frozenScene->camera;currentMaterialAlpha=frozenScene->materialAlpha;renderInputs.ranges.clear();'
new='camera=frozenScene->camera;modelVisible=!frozenScene->upload.opaque.empty()||!frozenScene->upload.translucent.empty();currentMaterialAlpha=frozenScene->materialAlpha;renderInputs.ranges.clear();'
assert old in s;s=s.replace(old,new,1)
p.write_text(s)
