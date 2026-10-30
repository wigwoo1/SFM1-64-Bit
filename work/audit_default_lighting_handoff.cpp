// Observe real prepared inputs and command payloads. No Vulkan calls or SFM process.
#define NOMINMAX
#define VK_NO_PROTOTYPES
#define DMX_GRAPH_NO_MAIN
#include <windows.h>
#include "dmx_graph.cpp"
#include "retained_session_scene_candidate.h"
#include "session_render_root_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include "prepared_scene_gpu_bridge_candidate.h"
#include <iomanip>
#include <iostream>

static void need(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
static std::string jsonString(const std::string& input){
    std::string out="\"";const char* hex="0123456789abcdef";
    for(unsigned char c:input){if(c=='"'||c=='\\'){out+='\\';out+=char(c);}else if(c<32){out+="\\u00";out+=hex[c>>4];out+=hex[c&15];}else out+=char(c);}return out+'"';
}
struct AuditImages {
    struct Entry{VkDescriptorSet set;};std::vector<Entry>entries;
    void upload(VkCommandBuffer){throw std::runtime_error("CPU audit must not upload GPU resources");}
};
struct AuditDraw {VkDescriptorSet descriptor{};VkPipeline pipeline{};std::array<float,12>material{};uint32_t first=0,count=0;};
struct Observer {AuditDraw pending;bool hasMaterial=false;unsigned cameraPushes=0;std::vector<AuditDraw>draws;};
static Observer* active=nullptr;
static void VKAPI_CALL pipeline(VkCommandBuffer,VkPipelineBindPoint,VkPipeline value){active->pending.pipeline=value;}
static void VKAPI_CALL descriptors(VkCommandBuffer,VkPipelineBindPoint,VkPipelineLayout,uint32_t first,uint32_t count,const VkDescriptorSet*sets,uint32_t dynamic,const uint32_t*){
    need(first==0&&count==1&&dynamic==0,"default descriptor command shape");active->pending.descriptor=sets[0];active->hasMaterial=false;
}
static void VKAPI_CALL vertices(VkCommandBuffer,uint32_t first,uint32_t count,const VkBuffer*,const VkDeviceSize*offset){need(first==0&&count==1&&offset[0]==0,"default vertex binding");}
static void VKAPI_CALL indices(VkCommandBuffer,VkBuffer,VkDeviceSize offset,VkIndexType type){need(offset==0&&type==VK_INDEX_TYPE_UINT32,"default index binding");}
static void VKAPI_CALL push(VkCommandBuffer,VkPipelineLayout,VkShaderStageFlags stage,uint32_t offset,uint32_t count,const void*data){
    if(stage==VK_SHADER_STAGE_VERTEX_BIT){need(offset==0&&count==64,"default camera push");++active->cameraPushes;return;}
    need(stage==VK_SHADER_STAGE_FRAGMENT_BIT&&offset==96&&count==48,"default material payload shape");
    std::memcpy(active->pending.material.data(),data,48);active->hasMaterial=true;
}
static void VKAPI_CALL draw(VkCommandBuffer,uint32_t count,uint32_t instances,uint32_t first,int32_t vertex,uint32_t instance){
    need(active->hasMaterial&&instances==1&&vertex==0&&instance==0,"default draw payload ordering");
    active->pending.first=first;active->pending.count=count;active->draws.push_back(active->pending);
}

int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 audit required");
    need(argc==3,"usage: audit disposable-session.dmx gameinfo.txt");
    auto graph=loadGraph(argv[1]);const auto root=sessionRenderRootCandidate(graph);
    const auto bytes=readBinaryAssetCandidate(argv[2]);const auto content=std::filesystem::path(argv[2]).parent_path();
    const auto mounts=gameinfoMountsCandidate(std::string(bytes.begin(),bytes.end()),content,content.parent_path());
    RetainedSessionSceneCandidate owner(std::move(graph),root,mounts,4,true,true);
    uintptr_t nextDescriptor=100;
    RetainedSceneMaterialGpuCacheCandidate<AuditImages>cache([&](const auto&input){auto images=std::make_unique<AuditImages>();for(size_t n=0;n<input.materials.size();++n)images->entries.push_back({VkDescriptorSet(nextDescriptor++)});return images;});
    PreparedSceneGpuBridgeCandidate<AuditImages>bridge(cache);
    std::cout<<std::setprecision(9);
    unsigned frames=0;size_t totalDraws=0;
    for(int32_t tick:{49583,50000,50417}){
        auto frame=owner.sample(tick,512,288);
        const auto resources=bridge.resources(frame,VkBuffer(1),VkBuffer(2),frame.upload.vertices.size()*sizeof(SceneGpuVertexCandidate),frame.upload.indices.size()*4,VkPipeline(10),VkPipeline(20),VkPipelineLayout(1));
        need(resources.selfIllumPush.empty()&&resources.materialPipelines.empty()&&resources.twoTextureSlots.empty(),"unexpected default diagnostic payload");
        Observer observer;active=&observer;
        bridge.record(frame,{pipeline,descriptors,vertices,indices,draw},push,VkCommandBuffer(1),resources);active=nullptr;
        std::vector<SceneGpuDrawCandidate>expected=frame.upload.opaque;expected.insert(expected.end(),frame.upload.translucent.begin(),frame.upload.translucent.end());
        need(observer.draws.size()==expected.size()&&observer.cameraPushes==1,"default command coverage");
        size_t vertexLit=0,unlit=0,world=0;
        std::cout<<"{\"kind\":\"frame\",\"tick\":"<<tick<<",\"shot\":"<<frame.shot<<",\"projected_lights\":"<<frame.lightSnapshots.size()<<",\"material_groups\":"<<frame.materialInputs.size()<<",\"material_slots\":"<<frame.upload.materials.size()<<",\"vertices\":"<<frame.upload.vertices.size()<<",\"indices\":"<<frame.upload.indices.size()<<",\"draws\":"<<expected.size()<<"}\n";
        for(size_t slot=0;slot<frame.upload.materials.size();++slot){
            const auto address=frame.upload.materials[slot];const auto&m=frame.materialInputs.at(address.model).materials.at(address.material);const auto shader=vmtAsciiLowerCandidate(m.source.syntax.key);
            if(shader=="vertexlitgeneric"){++vertexLit;need(!m.vertexLighting,"actor incorrectly labeled BSP vertex lighting");}
            else if(shader=="unlitgeneric"||shader=="unlittwotexture")++unlit;
            if(m.vertexLighting)++world;
            std::cout<<"{\"kind\":\"material\",\"tick\":"<<tick<<",\"slot\":"<<slot<<",\"group\":"<<address.model<<",\"index\":"<<address.material<<",\"shader\":"<<jsonString(shader)<<",\"vmt\":"<<jsonString(m.source.vmt.string())<<",\"base_texture\":"<<jsonString(m.source.baseTexture.string())<<",\"second_texture\":"<<jsonString(m.source.baseTexture2.string())<<",\"vertex_lighting\":"<<resources.vertexLighting[slot]<<",\"vertex_transition\":"<<resources.vertexTransition[slot]<<",\"selfillum_input\":"<<m.selfIllum.enabled<<",\"unlit_two_texture_input\":"<<m.unlitTwoTexture<<",\"declared\":{";
            bool comma=false;
            for(const char*name:{"$phong","$lightwarptexture","$rimlight","$bumpmap","$halflambert","$color","$color2","$selfillum","$basemapalphaphongmask"})for(const auto&field:m.source.syntax.children)if(vmtAsciiLowerCandidate(field.key)==name){if(comma)std::cout<<',';std::cout<<jsonString(name)<<':'<<jsonString(field.block?"<block>":field.value);comma=true;}
            std::cout<<"}}\n";
        }
        for(size_t n=0;n<expected.size();++n){
            const auto&source=expected[n];const auto&captured=observer.draws[n];const auto slot=source.materialSlot;
            need(captured.descriptor==resources.materials[slot]&&captured.first==source.firstIndex&&captured.count==source.indexCount,"draw/descriptor/material identity");
            need(captured.pipeline==(n<frame.upload.opaque.size()?VkPipeline(10):VkPipeline(20)),"default material pipeline selection");
            const auto&v=captured.material;need(v[0]==frame.materialAlpha[slot]&&v[1]==resources.alphaCutoff[slot]&&v[2]==resources.vertexLighting[slot]&&v[3]==resources.vertexTransition[slot],"default material push identity");
            need(std::equal(v.begin()+4,v.end(),resources.baseTextureTransform[slot].begin()),"default UV push identity");
            std::cout<<"{\"kind\":\"draw\",\"tick\":"<<tick<<",\"slot\":"<<slot<<",\"first_index\":"<<captured.first<<",\"index_count\":"<<captured.count<<",\"fragment_offset\":96,\"fragment_bytes\":48,\"alpha\":"<<v[0]<<",\"cutoff\":"<<v[1]<<",\"vertex_lighting\":"<<v[2]<<",\"vertex_transition\":"<<v[3]<<"}\n";
        }
        need(vertexLit==3&&unlit==2&&world==59&&frame.lightSnapshots.empty(),"benchmark lighting inventory changed");
        ++frames;totalDraws+=observer.draws.size();
    }
    std::cout<<"{\"kind\":\"result\",\"status\":\"VERIFIED\",\"frames\":"<<frames<<",\"observed_draws\":"<<totalDraws<<",\"gpu_calls\":0,\"native_parity\":\"UNVERIFIED\"}\n";
    return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
