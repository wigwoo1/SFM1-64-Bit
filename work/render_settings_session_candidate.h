#pragma once
#include "render_settings_candidate.h"
#include "session_render_root_candidate.h"
#include <vector>
#include <optional>
#include <cstring>
#include <limits>
inline constexpr const char* renderSettingsAttributeCandidate="sfm64RenderSettingsV1";
inline std::vector<uint8_t> encodeRenderSettingsCandidate(const PlaybackRenderSettingsCandidate&settings){
 static_assert(sizeof(double)==8&&std::numeric_limits<double>::is_iec559,"IEEE binary64 required");
 (void)renderSettingsJsonCandidate(settings);
 std::vector<uint8_t> bytes{'S','F','M','R','N','D','0','1'};
 auto append=[&](uint64_t value,unsigned count){for(unsigned i=0;i<count;++i)bytes.push_back(uint8_t(value>>(i*8)));};
 append(uint32_t(settings.renderer),4);append(uint32_t(settings.resolution),4);append(uint32_t(settings.samples),4);
 uint64_t noise;std::memcpy(&noise,&settings.noiseThreshold,8);append(noise,8);append((settings.adaptive?1u:0u)|(settings.denoising?2u:0u),4);return bytes;
}
inline PlaybackRenderSettingsCandidate decodeRenderSettingsCandidate(const std::vector<uint8_t>&bytes){
 if(bytes.size()!=32||std::memcmp(bytes.data(),"SFMRND01",8))throw std::runtime_error("Invalid render settings snapshot");
 auto read=[&](size_t offset,unsigned count){uint64_t value=0;for(unsigned i=0;i<count;++i)value|=uint64_t(bytes.at(offset+i))<<(i*8);return value;};
 auto renderer=read(8,4),resolution=read(12,4),samples=read(16,4),noise=read(20,8),flags=read(28,4);
 if(renderer>2||resolution>2||samples>uint64_t(INT32_MAX)||flags>3)throw std::runtime_error("Unsupported render settings snapshot fields");
 PlaybackRenderSettingsCandidate result;result.renderer=int(renderer);result.resolution=int(resolution);result.samples=int(samples);
 std::memcpy(&result.noiseThreshold,&noise,8);result.adaptive=(flags&1)!=0;result.denoising=(flags&2)!=0;
 (void)renderSettingsJsonCandidate(result);return result;
}
inline void embedRenderSettingsCandidate(Graph&graph,const PlaybackRenderSettingsCandidate&settings){
 auto bytes=encodeRenderSettingsCandidate(settings);
 auto&attributes=graph.elements.at(sessionRenderRootCandidate(graph)).attributes;
 size_t matches=0;std::vector<Attribute> staged;staged.reserve(attributes.size()+1);
 for(const auto&attribute:attributes){if(graph.strings.at(attribute.name)==renderSettingsAttributeCandidate)++matches;else staged.push_back(attribute);}
 if(matches>1)throw std::runtime_error("Duplicate render settings snapshots");
 Attribute replacement{};replacement.type=6;Value value;value.wire.assign(bytes.begin(),bytes.end());replacement.values.push_back(std::move(value));staged.push_back(std::move(replacement));
 staged.back().name=internString(graph,renderSettingsAttributeCandidate);attributes.swap(staged);
}
inline std::optional<PlaybackRenderSettingsCandidate> readRenderSettingsCandidate(const Graph&graph){
 const Attribute*found=nullptr;
 for(const auto&attribute:graph.elements.at(sessionRenderRootCandidate(graph)).attributes)if(graph.strings.at(attribute.name)==renderSettingsAttributeCandidate){if(found)throw std::runtime_error("Duplicate render settings snapshots");found=&attribute;}
 if(!found)return {};if(found->type!=6||found->values.size()!=1)throw std::runtime_error("Invalid render settings attribute");
 const auto&wire=found->values[0].wire;return decodeRenderSettingsCandidate(std::vector<uint8_t>(wire.begin(),wire.end()));
}
