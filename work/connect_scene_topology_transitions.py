from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
start=s.index('if(next.upload.vertices.size()!=vertices.size()')
end=s.index('*frozenScene=std::move(next);',start)
s=s[:start]+s[end:]
old='if(!sceneTicks.empty()){double scenePreparationMs=0;'
new='if(!sceneTicks.empty()){auto previousIndices=renderInputs.indices;double scenePreparationMs=0;'
assert old in s;s=s.replace(old,new,1)
old='}for(size_t n=0;n<vertices.size();++n){const auto&v=frozenScene->upload.vertices[n];'
new='}vertices.resize(frozenScene->upload.vertices.size());renderInputs.indices=frozenScene->upload.indices;materialScene.materials.clear();renderInputs.materials.clear();for(const auto&a:frozenScene->upload.materials){materialScene.materials.push_back({a.model,a.material});const auto&source=frozenScene->materialInputs.at(a.model).materials.at(a.material);ModelRenderMaterialCandidate metadata;metadata.source=source.source;metadata.translucent=source.translucent;renderInputs.materials.push_back(std::move(metadata));}for(size_t n=0;n<vertices.size();++n){const auto&v=frozenScene->upload.vertices[n];'
assert old in s;s=s.replace(old,new,1)
old='void*vertexMapped=nullptr;check(map(dev,vertexMemory,0,vertices.size()*sizeof(Vertex),0,&vertexMapped));std::memcpy(vertexMapped,vertices.data(),vertices.size()*sizeof(Vertex));unmap(dev,vertexMemory);cpuPreparationMs=scenePreparationMs+'
new='''bool grew=false;auto releaseBuffer=[&](VkBuffer oldBuffer,VkDeviceMemory oldMemory){destroyBuffer(dev,oldBuffer,nullptr);buffers.erase(std::remove(buffers.begin(),buffers.end(),oldBuffer),buffers.end());freeMemory(dev,oldMemory,nullptr);memories.erase(std::remove(memories.begin(),memories.end(),oldMemory),memories.end());};size_t vertexBytes=vertices.size()*sizeof(Vertex),indexBytes=indices.size()*sizeof(uint32_t);if(vertexBytes>vertexCapacity){VkDeviceMemory nextMemory=VK_NULL_HANDLE;size_t nextCapacity=std::max(vertexBytes,vertexCapacity*2);auto nextBuffer=buffer(nextCapacity,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,nullptr,nextMemory);releaseBuffer(vertexBuffer,vertexMemory);vertexBuffer=nextBuffer;vertexMemory=nextMemory;vertexCapacity=nextCapacity;grew=true;}bool indexUpload=previousIndices!=indices;if(indexBytes>indexCapacity){VkDeviceMemory nextMemory=VK_NULL_HANDLE;size_t nextCapacity=std::max(indexBytes,indexCapacity*2);auto nextBuffer=buffer(nextCapacity,VK_BUFFER_USAGE_INDEX_BUFFER_BIT,nullptr,nextMemory);releaseBuffer(indexBuffer,indexMemory);indexBuffer=nextBuffer;indexMemory=nextMemory;indexCapacity=nextCapacity;indexUpload=true;grew=true;}if(grew)++geometryAllocations;if(vertexBytes){void*mapped=nullptr;check(map(dev,vertexMemory,0,vertexBytes,0,&mapped));std::memcpy(mapped,vertices.data(),vertexBytes);unmap(dev,vertexMemory);}if(indexUpload&&indexBytes){void*mapped=nullptr;check(map(dev,indexMemory,0,indexBytes,0,&mapped));std::memcpy(mapped,indices.data(),indexBytes);unmap(dev,indexMemory);}cpuPreparationMs=scenePreparationMs+'''
assert old in s;s=s.replace(old,new,1)
old='VkDeviceMemory vertexMemory,indexMemory,readbackMemory=VK_NULL_HANDLE;auto vertexBuffer='
new='size_t vertexCapacity=vertices.size()*sizeof(Vertex),indexCapacity=indices.size()*sizeof(uint32_t);VkDeviceMemory vertexMemory,indexMemory,readbackMemory=VK_NULL_HANDLE;auto vertexBuffer='
assert old in s;s=s.replace(old,new,1)
old='size_t preparationTasks=0,materialUploadBatches=0;'
assert old in s;s=s.replace(old,'size_t preparationTasks=0,materialUploadBatches=0,geometryAllocations=1;',1)
old='<<" geometry_allocations=1 material_uploads="'
assert old in s;s=s.replace(old,'<<" geometry_allocations="<<geometryAllocations<<" material_uploads="',1)
p.write_text(s)
