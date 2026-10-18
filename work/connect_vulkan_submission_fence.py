from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
old='auto idle=load<PFN_vkQueueWaitIdle>(library,"vkQueueWaitIdle");'
assert old in s
s=s.replace(old,old+'auto createFence=load<PFN_vkCreateFence>(library,"vkCreateFence");auto destroyFence=load<PFN_vkDestroyFence>(library,"vkDestroyFence");auto resetFences=load<PFN_vkResetFences>(library,"vkResetFences");auto waitFences=load<PFN_vkWaitForFences>(library,"vkWaitForFences");')
s=s.replace('bool pending=false;', 'bool pending=false;VkFence frameFence=VK_NULL_HANDLE;size_t submissionFenceWaits=0;')
s=s.replace('if(pool)destroyPool(dev,pool,nullptr);','if(frameFence)destroyFence(dev,frameFence,nullptr);if(pool)destroyPool(dev,pool,nullptr);')
s=s.replace('try{materialImages.create(', 'try{VkFenceCreateInfo fenceInfo{};fenceInfo.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;check(createFence(dev,&fenceInfo,nullptr,&frameFence));materialImages.create(')
old='check(submit(graphics,1,&submission,VK_NULL_HANDLE));pending=true;check(idle(graphics));pending=false;'
assert old in s
s=s.replace(old,'check(resetFences(dev,1,&frameFence));check(submit(graphics,1,&submission,frameFence));pending=true;check(waitFences(dev,1,&frameFence,VK_TRUE,30000000000ull));++submissionFenceWaits;pending=false;')
s=s.replace('gpu_wait=per_frame native_full_parity=unverified', 'gpu_wait=submission_fence fence_waits="<<submissionFenceWaits<<" queue_idle_waits=0 native_full_parity=unverified')
p.write_text(s)
