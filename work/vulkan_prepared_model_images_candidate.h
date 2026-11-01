#pragma once
#include "vulkan_prepared_texture_binding_candidate.h"

// Composes verified prepared texture leases with the existing four-binding
// material descriptors. The retained model-input cache still owns this object;
// its caller must complete submissions before destroying the cache/device.
class VulkanPreparedModelImagesCandidate {
    using Binding=VulkanPreparedTextureBindingCandidate;
    struct Selected {std::shared_ptr<const Binding>owner;std::optional<Binding::FrameLease>lease;};
    HMODULE library;VkDevice device;VkPhysicalDevice physical;
    VkDescriptorSetLayout textureLayout=VK_NULL_HANDLE;
    std::unique_ptr<VulkanMaterialImagesCandidate>legacy;
    std::vector<Selected>base,second;
public:
    struct DescriptorEntry {VkDescriptorSet set=VK_NULL_HANDLE;VkImageView view=VK_NULL_HANDLE;};
    std::vector<DescriptorEntry>entries,maskEntries;
    VkSampler sampler=VK_NULL_HANDLE;size_t preparedTextures=0;
    VulkanPreparedModelImagesCandidate(HMODULE l,VkDevice d,VkPhysicalDevice p):library(l),device(d),physical(p){}
    VulkanPreparedModelImagesCandidate(const VulkanPreparedModelImagesCandidate&)=delete;
    ~VulkanPreparedModelImagesCandidate(){legacy.reset();base.clear();second.clear();if(textureLayout)load<PFN_vkDestroyDescriptorSetLayout>(library,"vkDestroyDescriptorSetLayout")(device,textureLayout,nullptr);}
    void create(const std::vector<ModelRenderMaterialCandidate>&materials,VkDescriptorSetLayout materialLayout,
        bool mask,bool baseSrgb,bool materialColor,bool bcEnabled){
        if(legacy||textureLayout)throw std::runtime_error("prepared model images already created");
        base.resize(materials.size());second.resize(materials.size());entries.resize(materials.size());
        auto prepare=[&](const ModelPreparedTextureCandidate&input,bool srgb,Selected&out){
            if(!input.cpu)return;
            const auto format=input.cpu->image()->format();
            // The existing decoded route remains valid on devices whose logical
            // device did not enable BC. Never infer enablement from support alone.
            if((format==13||format==15)&&!bcEnabled)return;
            if(!textureLayout){VkDescriptorSetLayoutBinding b[2]{{0,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}};VkDescriptorSetLayoutCreateInfo ci{};ci.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;ci.bindingCount=2;ci.pBindings=b;VulkanMaterialImagesCandidate::check(load<PFN_vkCreateDescriptorSetLayout>(library,"vkCreateDescriptorSetLayout")(device,&ci,nullptr,&textureLayout));}
            VulkanTextureSamplerPolicyCandidate policy;policy.minFilter=policy.magFilter=VK_FILTER_LINEAR;
            policy.baseMip=std::min(input.sourceMip,input.cpu->image()->plan()[9]-1);
            if(input.fullMipSampling){policy.mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR;policy.maximumLod=VK_LOD_CLAMP_NONE;}else policy.fixedLod=0.f;
            if(!Binding::create(input.cpu,library,device,physical,textureLayout,policy,srgb,bcEnabled,out.owner))throw std::runtime_error("prepared model GPU texture refused");
            out.lease=out.owner->acquireFrame(input.frame);if(!out.lease)throw std::runtime_error("prepared model GPU texture frame");++preparedTextures;
        };
        for(size_t n=0;n<materials.size();++n){const auto&m=materials[n];const bool srgb=materialColor?m.colorSpace.baseSrgb:baseSrgb;prepare(m.preparedBase,srgb,base[n]);prepare(m.preparedSecond,srgb,second[n]);
            if(base[n].lease&&second[n].lease&&m.preparedBase.fullMipSampling!=m.preparedSecond.fullMipSampling)throw std::runtime_error("material channels require one compatible sampler policy");}
        // Only tiny unused placeholders are allocated for replaced channels.
        // Mask and fallback images keep their unchanged owned legacy route.
        const VtfRgbaImageCandidate placeholder{1,1,{0,0,0,255}};
        struct MaterialView {const VtfRgbaImageCandidate&image;const MaterialSelfIllumCandidate&selfIllum;const VtfRgbaImageCandidate&selfIllumMaskImage;const MaterialColorSpaceCandidate&colorSpace;const VtfRgbaImageCandidate&secondBaseImage;};
        legacy=std::make_unique<VulkanMaterialImagesCandidate>(library,device,physical);
        legacy->createFrom(materials.size(),[&](size_t n){const auto&m=materials[n];return MaterialView{base[n].lease?placeholder:m.image,m.selfIllum,m.selfIllumMaskImage,m.colorSpace,second[n].lease?placeholder:m.secondBaseImage};},materialLayout,mask,baseSrgb,materialColor);
        sampler=legacy->sampler;
        for(size_t n=0;n<materials.size();++n){const auto&old=legacy->entries[n];entries[n]={old.set,base[n].lease?base[n].lease->imageView():old.view};VkWriteDescriptorSet writes[3]{};VkDescriptorImageInfo info[3]{};uint32_t count=0;
            auto write=[&](uint32_t binding,VkDescriptorType type,VkDescriptorImageInfo value){info[count]=value;auto&w=writes[count];w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=old.set;w.dstBinding=binding;w.descriptorCount=1;w.descriptorType=type;w.pImageInfo=&info[count++];};
            if(base[n].lease)write(0,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,{VK_NULL_HANDLE,base[n].lease->imageView(),VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
            if(second[n].lease)write(3,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,{VK_NULL_HANDLE,second[n].lease->imageView(),VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
            if(base[n].lease||second[n].lease){auto selected=base[n].lease?base[n].lease->sampler():second[n].lease->sampler();write(1,VK_DESCRIPTOR_TYPE_SAMPLER,{selected,VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED});if(!n)sampler=selected;}
            if(count)load<PFN_vkUpdateDescriptorSets>(library,"vkUpdateDescriptorSets")(device,count,writes,0,nullptr);
        }
        for(const auto&e:legacy->maskEntries)maskEntries.push_back({e.set,e.view});
    }
    void upload(VkCommandBuffer command){if(!legacy)throw std::runtime_error("prepared model images missing");legacy->upload(command);for(const auto*items:{&base,&second})for(const auto&selected:*items)if(selected.owner)selected.owner->recordUpload(command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);}
};
