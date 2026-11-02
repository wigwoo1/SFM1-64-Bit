#pragma once
#include "shader_texture_cpu_preparation_candidate.h"
#include "vulkan_material_images_candidate.h"
#include <cmath>

// Explicit backend sampler settings, not inferred Source flags or native-device
// policy. The full validated mip range is available to the caller's shader.
// Explicit promise from logical-device creation, not physical support alone.
struct VulkanTextureSamplerFeaturesCandidate {
    bool anisotropyEnabled=false;
    float maxAnisotropy=1.f;
};
struct VulkanTextureSamplerPolicyCandidate {
    VkFilter minFilter=VK_FILTER_NEAREST,magFilter=VK_FILTER_NEAREST;
    VkSamplerMipmapMode mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;
    VkSamplerAddressMode addressU=VK_SAMPLER_ADDRESS_MODE_REPEAT,addressV=VK_SAMPLER_ADDRESS_MODE_REPEAT;
    std::optional<float>fixedLod,maximumLod;
    uint32_t baseMip=0;
    std::optional<float>maxAnisotropy;
};
struct VulkanTextureBindingFunctionsCandidate {
    PFN_vkCreateSampler createSampler=nullptr;PFN_vkDestroySampler destroySampler=nullptr;
    PFN_vkCreateDescriptorPool createPool=nullptr;PFN_vkDestroyDescriptorPool destroyPool=nullptr;
    PFN_vkAllocateDescriptorSets allocateSets=nullptr;PFN_vkUpdateDescriptorSets updateSets=nullptr;
    PFN_vkGetPhysicalDeviceFormatProperties formatProperties=nullptr;
    static VulkanTextureBindingFunctionsCandidate fromLibrary(HMODULE library){
        return {load<PFN_vkCreateSampler>(library,"vkCreateSampler"),load<PFN_vkDestroySampler>(library,"vkDestroySampler"),
            load<PFN_vkCreateDescriptorPool>(library,"vkCreateDescriptorPool"),load<PFN_vkDestroyDescriptorPool>(library,"vkDestroyDescriptorPool"),
            load<PFN_vkAllocateDescriptorSets>(library,"vkAllocateDescriptorSets"),load<PFN_vkUpdateDescriptorSets>(library,"vkUpdateDescriptorSets"),
            load<PFN_vkGetPhysicalDeviceFormatProperties>(library,"vkGetPhysicalDeviceFormatProperties")};
    }
    bool valid()const noexcept{return createSampler&&destroySampler&&createPool&&destroyPool&&allocateSets&&updateSets&&formatProperties;}
};

// Prepared metadata + actual GPU images + immutable per-frame descriptors.
// Not a native manager/cache entry, raw MaterialTextureReference or GPU-complete
// flag. The device/loader/layout must outlive all leases. Caller records upload
// once and retains leases through queue completion; this class does not submit,
// wait a borrowed fence, or retire in-flight work on the caller's behalf.
class VulkanPreparedTextureBindingCandidate:public std::enable_shared_from_this<VulkanPreparedTextureBindingCandidate> {
    std::shared_ptr<const ShaderTextureCpuPreparationCandidate>cpu;
    std::unique_ptr<VulkanMaterialImagesCandidate>images;
    VulkanTextureBindingFunctionsCandidate functions{};
    VkDevice device=VK_NULL_HANDLE;VkDescriptorPool pool=VK_NULL_HANDLE;VkSampler sampler=VK_NULL_HANDLE;
    std::vector<VkDescriptorSet>sets;
    VulkanTextureSamplerPolicyCandidate sampling{};
    VulkanPreparedTextureBindingCandidate()=default;
public:
    class FrameLease {
        std::shared_ptr<const VulkanPreparedTextureBindingCandidate>owner;
        uint32_t frame=0;
        friend class VulkanPreparedTextureBindingCandidate;
    public:
        VkDescriptorSet descriptor()const{return owner?owner->sets.at(frame):VK_NULL_HANDLE;}
        uint32_t frameIndex()const noexcept{return frame;}
        VkImageView imageView()const{return owner?owner->images->entries.at(frame).view:VK_NULL_HANDLE;}
        VkSampler sampler()const noexcept{return owner?owner->sampler:VK_NULL_HANDLE;}
        const std::shared_ptr<const ShaderTextureCpuPreparationCandidate>&preparation()const{if(!owner)throw std::runtime_error("empty prepared texture lease");return owner->cpu;}
    };
    VulkanPreparedTextureBindingCandidate(const VulkanPreparedTextureBindingCandidate&)=delete;
    ~VulkanPreparedTextureBindingCandidate(){if(pool)functions.destroyPool(device,pool,nullptr);if(sampler)functions.destroySampler(device,sampler,nullptr);}
    static bool create(const std::shared_ptr<const ShaderTextureCpuPreparationCandidate>&prepared,
        HMODULE library,VkDevice device,VkPhysicalDevice physical,VkDescriptorSetLayout layout,
        const VulkanTextureSamplerPolicyCandidate&policy,bool srgb,bool compressionBcEnabled,
        std::shared_ptr<const VulkanPreparedTextureBindingCandidate>&output,
        const VulkanTextureBindingFunctionsCandidate&functions,
        const VulkanTextureSamplerFeaturesCandidate&features={})noexcept{
        try{
            const auto source=prepared;const auto calls=functions;const auto p=policy;
            if(!std::isfinite(features.maxAnisotropy)||features.maxAnisotropy<1.f)return false;
            if(p.maxAnisotropy&&(!features.anisotropyEnabled||!std::isfinite(*p.maxAnisotropy)||*p.maxAnisotropy<1.f||*p.maxAnisotropy>features.maxAnisotropy))return false;
            if((p.fixedLod&&(!std::isfinite(*p.fixedLod)||*p.fixedLod<0))||(p.maximumLod&&(!std::isfinite(*p.maximumLod)||*p.maximumLod<0)))return false;
            const auto filter=[](VkFilter f){return f==VK_FILTER_NEAREST||f==VK_FILTER_LINEAR;};
            const auto address=[](VkSamplerAddressMode a){return a==VK_SAMPLER_ADDRESS_MODE_REPEAT||a==VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE||a==VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;};
            if(!source||!source->image()||!library||!device||!physical||!layout||!calls.valid()||!filter(p.minFilter)||!filter(p.magFilter)||
                (p.mipmapMode!=VK_SAMPLER_MIPMAP_MODE_NEAREST&&p.mipmapMode!=VK_SAMPLER_MIPMAP_MODE_LINEAR)||!address(p.addressU)||!address(p.addressV))return false;
            std::shared_ptr<const MaterialTextureUploadPlanCandidate>upload;
            if(!MaterialTextureUploadPlanCandidate::prepare(source->image(),1,source->image()->plan()[9],source->image()->format(),upload))return false;
            auto next=std::shared_ptr<VulkanPreparedTextureBindingCandidate>(new VulkanPreparedTextureBindingCandidate);
            next->cpu=source;next->functions=calls;next->device=device;next->sampling=p;
            next->images=std::make_unique<VulkanMaterialImagesCandidate>(library,device,physical);
            next->images->createUploadPlan(*upload,srgb,compressionBcEnabled);
            auto&entries=next->images->entries;if(entries.empty()||p.baseMip>=upload->mipCount())return false;
            if(p.baseMip)for(auto&e:entries){VkImageViewCreateInfo vi{};vi.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;vi.image=e.image;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=e.format;vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,p.baseMip,upload->mipCount()-p.baseMip,0,1};VkImageView view=VK_NULL_HANDLE;VulkanMaterialImagesCandidate::check(load<PFN_vkCreateImageView>(library,"vkCreateImageView")(device,&vi,nullptr,&view));load<PFN_vkDestroyImageView>(library,"vkDestroyImageView")(device,e.view,nullptr);e.view=view;}
            if(p.minFilter==VK_FILTER_LINEAR||p.magFilter==VK_FILTER_LINEAR||p.mipmapMode==VK_SAMPLER_MIPMAP_MODE_LINEAR){
                VkFormatProperties formats{};calls.formatProperties(physical,entries[0].format,&formats);
                if(!(formats.optimalTilingFeatures&VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))return false;
            }
            VkSamplerCreateInfo si{};si.sType=VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;si.minFilter=p.minFilter;si.magFilter=p.magFilter;si.mipmapMode=p.mipmapMode;si.addressModeU=p.addressU;si.addressModeV=p.addressV;si.addressModeW=VK_SAMPLER_ADDRESS_MODE_REPEAT;si.borderColor=VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;si.anisotropyEnable=p.maxAnisotropy?VK_TRUE:VK_FALSE;si.maxAnisotropy=p.maxAnisotropy.value_or(1.f);si.minLod=0;si.maxLod=p.maximumLod.value_or(float(upload->mipCount()-p.baseMip-1));if(p.fixedLod)si.minLod=si.maxLod=*p.fixedLod;
            VulkanMaterialImagesCandidate::check(calls.createSampler(device,&si,nullptr,&next->sampler));
            const uint32_t count=uint32_t(entries.size());VkDescriptorPoolSize sizes[2]{{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,count},{VK_DESCRIPTOR_TYPE_SAMPLER,count}};VkDescriptorPoolCreateInfo pi{};pi.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;pi.maxSets=count;pi.poolSizeCount=2;pi.pPoolSizes=sizes;VulkanMaterialImagesCandidate::check(calls.createPool(device,&pi,nullptr,&next->pool));
            next->sets.resize(count,VK_NULL_HANDLE);
            for(uint32_t frame=0;frame<count;++frame){VkDescriptorSetAllocateInfo ai{};ai.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;ai.descriptorPool=next->pool;ai.descriptorSetCount=1;ai.pSetLayouts=&layout;VulkanMaterialImagesCandidate::check(calls.allocateSets(device,&ai,&next->sets[frame]));
                VkDescriptorImageInfo descriptors[2]{{VK_NULL_HANDLE,entries[frame].view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},{next->sampler,VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED}};VkWriteDescriptorSet writes[2]{};
                for(uint32_t i=0;i<2;++i){writes[i].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;writes[i].dstSet=next->sets[frame];writes[i].dstBinding=i;writes[i].descriptorCount=1;writes[i].descriptorType=i?VK_DESCRIPTOR_TYPE_SAMPLER:VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;writes[i].pImageInfo=&descriptors[i];}
                calls.updateSets(device,2,writes,0,nullptr);
            }
            output=std::move(next);return true;
        }catch(...){return false;}
    }
    static bool create(const std::shared_ptr<const ShaderTextureCpuPreparationCandidate>&prepared,
        HMODULE library,VkDevice device,VkPhysicalDevice physical,VkDescriptorSetLayout layout,
        const VulkanTextureSamplerPolicyCandidate&policy,bool srgb,bool compressionBcEnabled,
        std::shared_ptr<const VulkanPreparedTextureBindingCandidate>&output,
        const VulkanTextureSamplerFeaturesCandidate&features={})noexcept{
        try{return create(prepared,library,device,physical,layout,policy,srgb,compressionBcEnabled,output,VulkanTextureBindingFunctionsCandidate::fromLibrary(library),features);}catch(...){return false;}
    }
    std::optional<FrameLease>acquireFrame(uint32_t frame)const{if(frame>=sets.size())return {};FrameLease lease;lease.owner=shared_from_this();lease.frame=frame;return lease;}
    void recordUpload(VkCommandBuffer command,VkPipelineStageFlags consumers)const{images->upload(command,consumers);}
    uint32_t frameCount()const noexcept{return uint32_t(sets.size());}
    uint32_t mipCount()const noexcept{return cpu->image()->plan()[9];}
    const std::shared_ptr<const ShaderTextureCpuPreparationCandidate>&preparation()const noexcept{return cpu;}
    const VulkanTextureSamplerPolicyCandidate&samplerPolicy()const noexcept{return sampling;}
};
