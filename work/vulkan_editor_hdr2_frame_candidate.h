#pragma once
#include "scene_hdr2_profile_candidate.h"
#include "vulkan_hdr2_lightmap_binding_candidate.h"

// One owner for CPU profile/plan, optional sampled resources and EVERY draw
// submission. Upload Ready does not mean a later sampling submission is done.
// Caller resets/destroys a discarded command before releasing a Recorded owner.
class VulkanEditorHdr2FrameCandidate {
public:
    enum class State {Idle,Recorded,InFlight};
private:
    HMODULE library_;VkDevice device_;State state_=State::Idle;
    VkCommandBuffer command_=VK_NULL_HANDLE;bool upload_=false;
    std::shared_ptr<const SceneHdr2PreparedFrameCandidate> frame_;
    std::shared_ptr<VulkanHdr2LightmapImagesCandidate> images_;
    std::shared_ptr<VulkanHdr2LightmapBindingCandidate> binding_;
    VulkanEditorHdr2FrameCandidate(HMODULE l,VkDevice d,std::shared_ptr<const SceneHdr2PreparedFrameCandidate> f):library_(l),device_(d),frame_(std::move(f)){}
public:
    VulkanEditorHdr2FrameCandidate(const VulkanEditorHdr2FrameCandidate&)=delete;
    ~VulkanEditorHdr2FrameCandidate(){if(state_==State::InFlight)load<PFN_vkDeviceWaitIdle>(library_,"vkDeviceWaitIdle")(device_);}
    State state()const noexcept{return state_;}
    const auto& frame()const noexcept{return frame_;}
    static std::unique_ptr<VulkanEditorHdr2FrameCandidate> prepare(HMODULE library,VkDevice device,VkPhysicalDevice physical,
            std::shared_ptr<const SceneHdr2PreparedFrameCandidate> frame,VkDescriptorSetLayout lightLayout){
        if(!library||!device||!physical||!frame||!lightLayout)throw std::runtime_error("HDR2 editor frame inputs");
        auto next=std::unique_ptr<VulkanEditorHdr2FrameCandidate>(new VulkanEditorHdr2FrameCandidate(library,device,std::move(frame)));
        if(!next->frame_->plan()->faces.empty()){
            next->images_=VulkanHdr2LightmapImagesCandidate::prepare(library,device,physical,next->frame_->plan());
            next->binding_=VulkanHdr2LightmapBindingCandidate::prepare(library,device,physical,next->images_,lightLayout,{},
                next->frame_->profile()->filter()==SceneHdr2FilterCandidate::Linear?VK_FILTER_LINEAR:VK_FILTER_NEAREST);
        }
        return next;
    }
    void recordUploads(VkCommandBuffer command){
        if(!command||state_!=State::Idle)throw std::runtime_error("HDR2 editor record state");
        const bool upload=images_&&images_->state()==VulkanHdr2LightmapImagesCandidate::State::Fresh;
        if(images_)images_->recordUpload(command);
        command_=command;upload_=upload;state_=State::Recorded;
    }
    void configure(VulkanPreparedSceneResourcesCandidate& resources,VkPipeline pipeline,VkCommandBuffer command,VkPipeline directional=VK_NULL_HANDLE,std::function<bool(const SceneGpuDrawCandidate&)> available={}){
        if(state_!=State::Recorded||command!=command_||!pipeline)throw std::runtime_error("HDR2 editor configure state");
        if(binding_)binding_->configure(resources,pipeline,command,directional,available);
    }
    void submitted(VkCommandBuffer command){
        if(state_!=State::Recorded||command!=command_)throw std::runtime_error("HDR2 editor submit state");
        if(upload_)images_->submitted(command);
        state_=State::InFlight;
    }
    void completed(){
        if(state_!=State::InFlight)throw std::runtime_error("HDR2 editor completion state");
        if(upload_)images_->completed();
        state_=State::Idle;command_=VK_NULL_HANDLE;upload_=false;
    }
    void discardAfterReset(VkCommandBuffer command){
        if(state_!=State::Recorded||command!=command_)throw std::runtime_error("HDR2 editor discard state");
        if(upload_)images_->discardRecording(command);
        state_=State::Idle;command_=VK_NULL_HANDLE;upload_=false;
    }
};
