from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text(encoding='utf-8')
needle='const bool staticFlex=staticFlexSize&&staticFlexText[0]==\'1\';'
assert s.count(needle)==1
s=s.replace(needle,needle+'char softwareFlexText[8]{};auto softwareFlexSize=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_SOFTWARE_FLEX",softwareFlexText,sizeof(softwareFlexText));if(softwareFlexSize&&(softwareFlexSize!=1||(softwareFlexText[0]!=\'0\'&&softwareFlexText[0]!=\'1\')))throw std::runtime_error("software flex option must be 0 or 1");const bool softwareFlex=softwareFlexSize&&softwareFlexText[0]==\'1\';if(softwareFlex&&!staticFlex)throw std::runtime_error("software flex requires static flex");',1)
needle='std::make_shared<RetainedStaticFlexProviderCandidate>(materialMounts)';assert s.count(needle)==1;s=s.replace(needle,'std::make_shared<RetainedStaticFlexProviderCandidate>(materialMounts,softwareFlex,softwareFlex)',1)
p.write_text(s,encoding='utf-8')
p=Path('work/build_vulkan_surface_probe.cmd');s=p.read_text(encoding='utf-8');assert '/fp:strict' not in s;s=s.replace('/std:c++17','/std:c++17 /fp:strict',1);p.write_text(s,encoding='utf-8')
