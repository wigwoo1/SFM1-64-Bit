#pragma once
#include "vulkan_prepared_texture_binding_candidate.h"

// Composes retained prepared texture leases with the six-binding normal
// material descriptors. The retained model-input cache still owns this object;
// its caller must complete submissions before destroying the cache/device.
class VulkanPreparedModelImagesCandidate {
    using Binding=VulkanPreparedTextureBindingCandidate;
    struct Selected {std::shared_ptr<const Binding>owner;std::optional<Binding::FrameLease>lease;};
    HMODULE library;VkDevice device;VkPhysicalDevice physical;
    VkDescriptorSetLayout textureLayout=VK_NULL_HANDLE;
    std::unique_ptr<VulkanMaterialImagesCandidate>legacy;
    std::vector<Selected>base,second;
    struct LightwarpBindings { Selected warp,normal,exponent; };
    std::vector<LightwarpBindings> lightwarp;
public:
    struct DescriptorEntry {VkDescriptorSet set=VK_NULL_HANDLE;VkImageView view=VK_NULL_HANDLE;};
    std::vector<DescriptorEntry>entries,maskEntries;
    VkSampler sampler=VK_NULL_HANDLE;size_t preparedTextures=0;
    VulkanPreparedModelImagesCandidate(HMODULE l,VkDevice d,VkPhysicalDevice p):library(l),device(d),physical(p){}
    VulkanPreparedModelImagesCandidate(const VulkanPreparedModelImagesCandidate&)=delete;
    ~VulkanPreparedModelImagesCandidate(){legacy.reset();lightwarp.clear();base.clear();second.clear();if(textureLayout)load<PFN_vkDestroyDescriptorSetLayout>(library,"vkDestroyDescriptorSetLayout")(device,textureLayout,nullptr);}
    void create(const std::vector<ModelRenderMaterialCandidate>&materials,VkDescriptorSetLayout materialLayout,
        bool mask,bool baseSrgb,bool materialColor,bool bcEnabled,const VulkanTextureSamplerFeaturesCandidate&samplingFeatures={},bool lightwarpLayout=false){
        if(legacy||textureLayout)throw std::runtime_error("prepared model images already created");
        // Exact preceding-policy control for isolated rendered regressions.
        char fixedOption[8]{};const auto fixedLength=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_FIXED_TEXTURE_MIP_TEST",fixedOption,sizeof(fixedOption));
        if(fixedLength&&(fixedLength!=1||fixedOption[0]!='1'))throw std::runtime_error("fixed texture mip test option");
        const bool fixedMipTest=fixedLength!=0;
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
            // Enabled reference preset, not live CVar/hardware negotiation. Original
            // border flag overrides S/T clamp; reset border color is transparent
            // black. Each prepared channel keeps its own immutable sampler.
            const auto flags=input.cpu->image()->plan()[0];
            if(flags&1u)policy.minFilter=policy.magFilter=VK_FILTER_NEAREST;
            if(flags&4u)policy.addressU=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            if(flags&8u)policy.addressV=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            if(flags&0x20000000u)policy.addressU=policy.addressV=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            // Diagnostic force1 policy. Raw global force is NOT the effective max.
            // Vulkan directional pixels differ from D3D9; caller enables explicitly.
            const bool anisotropic=samplingFeatures.anisotropyEnabled&&samplingFeatures.maxAnisotropy>=2.f&&!fixedMipTest&&(flags&0x10u)&&!(flags&(1u|0x100u));
            if(anisotropic)policy.maxAnisotropy=std::min(samplingFeatures.maxAnisotropy,std::max(2.f,std::min(8.f,std::floor(samplingFeatures.maxAnisotropy/4.f))));
            if((input.fullMipSampling||anisotropic)&&!fixedMipTest&&!(flags&(1u|0x100u))){policy.mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR;policy.maximumLod=VK_LOD_CLAMP_NONE;}else policy.fixedLod=0.f;
            if(!Binding::create(input.cpu,library,device,physical,textureLayout,policy,srgb,bcEnabled,out.owner,samplingFeatures))throw std::runtime_error("prepared model GPU texture refused");
            out.lease=out.owner->acquireFrame(input.frame);if(!out.lease)throw std::runtime_error("prepared model GPU texture frame");++preparedTextures;
        };
        for(size_t n=0;n<materials.size();++n){const auto&m=materials[n];const bool srgb=materialColor?m.colorSpace.baseSrgb:baseSrgb;prepare(m.preparedBase,srgb,base[n]);prepare(m.preparedSecond,srgb,second[n]);}
        for(size_t n=0;n<materials.size();++n){
            const auto&inputs=materials[n].lightwarpInputs;
            // LightmappedGeneric and VertexLitGeneric use the same linear auxiliary image slot.
            if(lightwarpLayout && materials[n].selfShadowedBump.cpu){
                if(lightwarp.empty())lightwarp.resize(materials.size());
                prepare(materials[n].selfShadowedBump,false,lightwarp[n].normal);
                continue;
            }
            if(!inputs.warp.cpu)continue;
            if(lightwarp.empty())lightwarp.resize(materials.size());
            auto&bindings=lightwarp[n];
            prepare(inputs.warp,false,bindings.warp);
            if(!bindings.warp.lease)continue;
            prepare(inputs.normal,false,bindings.normal);
            prepare(inputs.exponent,false,bindings.exponent);
        }
        // Only tiny unused placeholders are allocated for replaced channels.
        // Mask and fallback images keep their unchanged owned legacy route.
        const VtfRgbaImageCandidate placeholder{1,1,{0,0,0,255}};
        struct MaterialView {const VtfRgbaImageCandidate&image;const MaterialSelfIllumCandidate&selfIllum;const VtfRgbaImageCandidate&selfIllumMaskImage;const MaterialColorSpaceCandidate&colorSpace;const VtfRgbaImageCandidate&secondBaseImage;};
        legacy=std::make_unique<VulkanMaterialImagesCandidate>(library,device,physical);
        legacy->createFrom(materials.size(),[&](size_t n){const auto&m=materials[n];return MaterialView{base[n].lease?placeholder:m.image,m.selfIllum,m.selfIllumMaskImage,m.colorSpace,second[n].lease?placeholder:m.secondBaseImage};},materialLayout,mask,baseSrgb,materialColor,0,true,lightwarpLayout);
        sampler=legacy->sampler;
        for(size_t n=0;n<materials.size();++n){const auto&old=legacy->entries[n];entries[n]={old.set,base[n].lease?base[n].lease->imageView():old.view};VkWriteDescriptorSet writes[10]{};VkDescriptorImageInfo info[10]{};uint32_t count=0;
            auto write=[&](uint32_t binding,VkDescriptorType type,VkDescriptorImageInfo value){info[count]=value;auto&w=writes[count];w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=old.set;w.dstBinding=binding;w.descriptorCount=1;w.descriptorType=type;w.pImageInfo=&info[count++];};
            if(base[n].lease)write(0,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,{VK_NULL_HANDLE,base[n].lease->imageView(),VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
            if(second[n].lease)write(3,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,{VK_NULL_HANDLE,second[n].lease->imageView(),VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
            if(base[n].lease){write(1,VK_DESCRIPTOR_TYPE_SAMPLER,{base[n].lease->sampler(),VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED});if(!n)sampler=base[n].lease->sampler();}
            if(second[n].lease)write(5,VK_DESCRIPTOR_TYPE_SAMPLER,{second[n].lease->sampler(),VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED});
            // HDR2 model slots 6..8 and 9..11 carry normal, exponent and warp
            // views and their respective VTF samplers in the material set.
            if(lightwarpLayout && !lightwarp.empty()){
                const auto& channels=lightwarp[n];
                uint32_t slot=6;
                for(const auto* channel:{&channels.normal,&channels.exponent,&channels.warp}){
                    if(channel->lease){
                        write(slot,VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,{VK_NULL_HANDLE,channel->lease->imageView(),VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
                        write(slot+3,VK_DESCRIPTOR_TYPE_SAMPLER,{channel->lease->sampler(),VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED});
                    }
                    ++slot;
                }
            }
            // Mask binding4 keeps its own legacy policy; a missing base must
            // never inherit the prepared second texture sampler.
            if(count)load<PFN_vkUpdateDescriptorSets>(library,"vkUpdateDescriptorSets")(device,count,writes,0,nullptr);
        }
        for(const auto&e:legacy->maskEntries)maskEntries.push_back({e.set,e.view});
    }
    bool hasExponent(size_t index)const{return index<lightwarp.size() && bool(lightwarp[index].exponent.lease);}
    bool hasNormal(size_t index)const{return index<lightwarp.size() && bool(lightwarp[index].normal.lease);}
    bool hasWarp(size_t index)const{return index<lightwarp.size() && bool(lightwarp[index].warp.lease);}
    size_t lightwarpBindingCount()const{return std::count_if(lightwarp.begin(),lightwarp.end(),[](const auto& b){return bool(b.warp.lease);});}
    void upload(VkCommandBuffer command){if(!legacy)throw std::runtime_error("prepared model images missing");legacy->upload(command);
        for(const auto&bindings:lightwarp)
            for(const auto* channel:{&bindings.warp,&bindings.normal,&bindings.exponent})
                if(channel->owner)channel->owner->recordUpload(command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        for(const auto*items:{&base,&second})for(const auto&selected:*items)if(selected.owner)selected.owner->recordUpload(command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);}
};
