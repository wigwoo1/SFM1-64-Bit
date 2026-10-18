from pathlib import Path
p=Path('work/render_frame_provider_candidate.h');s=p.read_text().replace('#include <functional>','#include <functional>\n#include "vmt_keyvalues_candidate.h"\n#include <filesystem>')
s=s.replace('uint32_t initialSkin=0,initialBody=0;','uint32_t initialSkin=0,initialBody=0;std::string initialMaterialKey;std::function<void(const std::filesystem::path&,VmtNodeCandidate&)>applyMaterialOverrides;')
p.write_text(s)
p=Path('work/live_session_frame_provider_candidate.h');s=p.read_text().replace('#include "recovered_tick_seconds_candidate.h"','#include "recovered_tick_seconds_candidate.h"\n#include "session_material_binding_candidate.h"\n#include "gameinfo_mounts_candidate.h"')
s=s.replace('auto timeline=std::make_shared<RetainedShotTimelineCandidate>', 'auto initialOverrides=readSessionMaterialOverridesCandidate(graph,firstBinding.models.front());auto initialMaterialKey=sessionMaterialOverrideKeyCandidate(initialOverrides);auto timeline=std::make_shared<RetainedShotTimelineCandidate>')
s=s.replace('provider.initialBody=initialBody;provider.sample=', '''provider.initialBody=initialBody;provider.initialMaterialKey=initialMaterialKey;std::vector<std::filesystem::path>materialMounts{content};if(std::filesystem::is_regular_file(content/"gameinfo.txt")){std::ifstream file(content/"gameinfo.txt",std::ios::binary);std::string text((std::istreambuf_iterator<char>(file)),{});materialMounts=gameinfoMountsCandidate(text,content,content.parent_path());}provider.applyMaterialOverrides=sessionMaterialBindingCandidate(initialOverrides,materialMounts);std::cout<<"live_material_override_records="<<initialOverrides.size()<<" native_precedence=unverified\\n";provider.sample=''')
s=s.replace('expected,initialSkin,initialBody]', 'expected,initialSkin,initialBody,initialMaterialKey]')
s=s.replace('auto id=binding.models.front();','auto id=binding.models.front();if(sessionMaterialOverrideKeyCandidate(readSessionMaterialOverridesCandidate(g,id))!=initialMaterialKey)throw std::runtime_error("live material overrides changed; material rebuild required");')
p.write_text(s)
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text();old='modelBodySelectionCandidate(selectedBody,model.bodyModelCounts,model.bodyBases));';new='modelBodySelectionCandidate(selectedBody,model.bodyModelCounts,model.bodyBases),frameProvider.applyMaterialOverrides);'
if s.count(old)!=1:raise RuntimeError('render input call')
p.write_text(s.replace(old,new))
