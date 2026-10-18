#pragma once
#include <sstream>
#include <locale>
#include <iomanip>
#include <stdexcept>
#include <cmath>
struct PlaybackRenderSettingsCandidate {
 int renderer=1; // Source, Vulkan, Cycles
 int resolution=0; // 1080p, 1440p, 4K
 int samples=128;
 double noiseThreshold=.01;
 bool adaptive=true,denoising=true;
};
inline bool sameRenderSettingsCandidate(const PlaybackRenderSettingsCandidate&a,const PlaybackRenderSettingsCandidate&b)noexcept{return a.renderer==b.renderer&&a.resolution==b.resolution&&a.samples==b.samples&&a.noiseThreshold==b.noiseThreshold&&a.adaptive==b.adaptive&&a.denoising==b.denoising;}
inline std::string renderSettingsJsonCandidate(const PlaybackRenderSettingsCandidate& settings){
 if(settings.renderer<0||settings.renderer>2||settings.resolution<0||settings.resolution>2||settings.samples<1||!std::isfinite(settings.noiseThreshold)||settings.noiseThreshold<0||settings.noiseThreshold>1)throw std::runtime_error("Invalid render settings");
 const char*renderer[]={"source","vulkan","cycles"};
 const int width[]={1920,2560,3840},height[]={1080,1440,2160};
 std::ostringstream output;output.imbue(std::locale::classic());output<<std::setprecision(17);
 output<<"{\"renderer\":\""<<renderer[settings.renderer]<<"\",\"resolution\":["<<width[settings.resolution]<<','<<height[settings.resolution]<<"],\"cycles\":{\"samples\":"<<settings.samples<<",\"adaptive_threshold\":"<<settings.noiseThreshold<<",\"use_adaptive_sampling\":"<<(settings.adaptive?"true":"false")<<",\"use_denoising\":"<<(settings.denoising?"true":"false")<<"}}";
 return output.str();
}
