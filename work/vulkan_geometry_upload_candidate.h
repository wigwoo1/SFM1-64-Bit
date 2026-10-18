#pragma once
#include "sfm_model_geometry.h"
#include "parallel_geometry_deformation.h"
#include <fstream>
struct VkProbeBufferInfo{uint32_t type;const void*next;uint32_t flags;uint64_t size;uint32_t usage,sharing,count;const uint32_t*families;};
struct VkProbeMemoryRequirements{uint64_t size,alignment;uint32_t bits;};
struct VkProbeMemoryAllocateInfo{uint32_t type;const void*next;uint64_t size;uint32_t memoryType;};
struct VkProbeMemoryType{uint32_t flags,heap;};
struct VkProbeMemoryHeap{uint64_t size;uint32_t flags;};
struct VkProbeMemoryProperties{uint32_t typeCount;VkProbeMemoryType types[32];uint32_t heapCount;VkProbeMemoryHeap heaps[16];};
using VkProbeCreateBuffer=int32_t(WINAPI*)(Handle,const VkProbeBufferInfo*,const void*,uint64_t*);
using VkProbeDestroyBuffer=void(WINAPI*)(Handle,uint64_t,const void*);
using VkProbeGetRequirements=void(WINAPI*)(Handle,uint64_t,VkProbeMemoryRequirements*);
using VkProbeGetMemoryProperties=void(WINAPI*)(Handle,VkProbeMemoryProperties*);
using VkProbeAllocateMemory=int32_t(WINAPI*)(Handle,const VkProbeMemoryAllocateInfo*,const void*,uint64_t*);
using VkProbeFreeMemory=void(WINAPI*)(Handle,uint64_t,const void*);
using VkProbeBindMemory=int32_t(WINAPI*)(Handle,uint64_t,uint64_t,uint64_t);
using VkProbeMapMemory=int32_t(WINAPI*)(Handle,uint64_t,uint64_t,uint64_t,uint32_t,void**);
using VkProbeUnmapMemory=void(WINAPI*)(Handle,uint64_t);
#include "vulkan_buffer_copy_candidate.h"
inline void testVulkanGeometryUploadCandidate(HMODULE library,Handle physical,Handle device,Handle queue,uint32_t family,const std::string&base){
 static_assert(sizeof(VkProbeBufferInfo)==56&&sizeof(VkProbeMemoryRequirements)==24&&sizeof(VkProbeMemoryAllocateInfo)==32&&sizeof(VkProbeMemoryProperties)==520,"Vulkan memory ABI");
 auto read=[](const std::string&path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("model fixture missing");return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),{});};
 auto model=readSfmModelGeometryCandidate(read(base+".mdl"),read(base+".vvd"),read(base+".dx90.vtx"));std::vector<GeometryMatrixCandidate>locals;for(auto&bone:model.bones)locals.push_back(bone.local);auto palette=evaluateGeometryPosePaletteCandidate(model.bones,locals);auto deformed=deformGeometryBatchCandidate(model.geometry,palette,4);
 struct Vertex{std::array<float,3>position,normal;std::array<float,2>uv;};static_assert(sizeof(Vertex)==32,"vertex layout");std::vector<Vertex>vertices;vertices.reserve(deformed.size());for(auto&v:deformed)vertices.push_back({v.position,v.normal,v.uv});std::vector<uint32_t>indices;for(auto&mesh:model.meshes)for(auto&triangle:mesh.triangles)for(auto v:triangle)indices.push_back(v);
 auto create=load<VkProbeCreateBuffer>(library,"vkCreateBuffer");auto destroy=load<VkProbeDestroyBuffer>(library,"vkDestroyBuffer");auto requirements=load<VkProbeGetRequirements>(library,"vkGetBufferMemoryRequirements");auto properties=load<VkProbeGetMemoryProperties>(library,"vkGetPhysicalDeviceMemoryProperties");auto allocate=load<VkProbeAllocateMemory>(library,"vkAllocateMemory");auto free=load<VkProbeFreeMemory>(library,"vkFreeMemory");auto bind=load<VkProbeBindMemory>(library,"vkBindBufferMemory");auto map=load<VkProbeMapMemory>(library,"vkMapMemory");auto unmap=load<VkProbeUnmapMemory>(library,"vkUnmapMemory");VkProbeMemoryProperties memoryProperties{};properties(physical,&memoryProperties);if(memoryProperties.typeCount>32)throw std::runtime_error("memory type count");
 auto upload=[&](const void*source,size_t bytes,uint32_t usage){uint64_t buffer=0,memory=0;try{if(!bytes)throw std::runtime_error("empty geometry buffer");VkProbeBufferInfo info{};info.type=12;info.size=bytes;info.usage=usage|1;auto result=create(device,&info,nullptr,&buffer);if(result)throw std::runtime_error("vkCreateBuffer "+std::to_string(result));VkProbeMemoryRequirements req{};requirements(device,buffer,&req);uint32_t type=32;for(uint32_t n=0;n<memoryProperties.typeCount;++n)if((req.bits&(uint32_t(1)<<n))&&(memoryProperties.types[n].flags&6)==6){type=n;break;}if(type==32)throw std::runtime_error("no host-visible coherent buffer memory");VkProbeMemoryAllocateInfo allocation{};allocation.type=5;allocation.size=req.size;allocation.memoryType=type;result=allocate(device,&allocation,nullptr,&memory);if(result)throw std::runtime_error("vkAllocateMemory "+std::to_string(result));result=bind(device,buffer,memory,0);if(result)throw std::runtime_error("vkBindBufferMemory "+std::to_string(result));void*mapped=nullptr;result=map(device,memory,0,bytes,0,&mapped);if(result)throw std::runtime_error("vkMapMemory "+std::to_string(result));std::memcpy(mapped,source,bytes);bool equal=std::memcmp(mapped,source,bytes)==0;unmap(device,memory);if(!equal)throw std::runtime_error("Vulkan mapped upload mismatch");std::cout<<"geometry_buffer_usage="<<usage<<" bytes="<<bytes<<" mapped_compare_exact=1 memory_type="<<type<<'\n';testVulkanBufferCopyCandidate(library,device,queue,family,buffer,source,bytes,memoryProperties);destroy(device,buffer,nullptr);buffer=0;free(device,memory,nullptr);memory=0;}catch(...){if(buffer)destroy(device,buffer,nullptr);if(memory)free(device,memory,nullptr);throw;}};
 upload(vertices.data(),vertices.size()*sizeof(Vertex),0x80);upload(indices.data(),indices.size()*4,0x40);std::cout<<"Vulkan geometry upload passed vertices="<<vertices.size()<<" indices="<<indices.size()<<'\n';
}
