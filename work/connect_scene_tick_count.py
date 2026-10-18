from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text();s='#include "scene_tick_sequence_candidate.h"\n'+s
start=s.index('std::ifstream input(sceneTickPath);if(!input)throw')
end=s.index('int32_t initialTick=0;',start)
s=s[:start]+'sceneTicks=readSceneTickSequenceCandidate(sceneTickPath);'+s[end:];p.write_text(s)
p=Path('work/vulkan_model_swapchain_probe_candidate.h');s=p.read_text()
old='size_t count=0;if(!countSize||countSize>=sizeof(countText))throw std::runtime_error("playback frame count missing");auto parsed=std::from_chars(countText,countText+countSize,count);if(parsed.ec!=std::errc{}||parsed.ptr!=countText+countSize||!count||count>10000)throw std::runtime_error("playback frame count");'
new='std::optional<size_t>explicitCount;if(countSize){if(countSize>=sizeof(countText))throw std::runtime_error("playback frame count length");size_t value=0;auto parsed=std::from_chars(countText,countText+countSize,value);if(parsed.ec!=std::errc{}||parsed.ptr!=countText+countSize)throw std::runtime_error("playback frame count");explicitCount=value;}char tickPath[32768]{};auto tickSize=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_SCENE_TICKS",tickPath,sizeof(tickPath));if(tickSize>=sizeof(tickPath))throw std::runtime_error("scene tick path length");auto ticks=tickSize?readSceneTickSequenceCandidate(tickPath):std::vector<int32_t>{};size_t count=scenePlaybackFrameCountCandidate(ticks,explicitCount);std::cout<<"playback_count_source="<<(ticks.empty()?"explicit":"scene_ticks")<<" count="<<count<<"\\n";'
assert old in s;s=s.replace(old,new,1);p.write_text(s)
