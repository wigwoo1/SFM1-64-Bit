from pathlib import Path
p=Path('work/model_flex_deformation_candidate.h');s=p.read_text(encoding='utf-8')
old='inline VvdGeometryCandidate deformModelFlexesCandidate('
assert s.count(old)==1
s=s.replace(old,'inline void deformModelFlexesIntoCandidate(',1)
s=s.replace('>&selected,ModelFlexWorkProfileCandidate*profile=nullptr){','>&selected,VvdGeometryCandidate&result,ModelFlexWorkProfileCandidate*profile=nullptr){',1)
s=s.replace(' if(!selected',' if(&baseline==&result)throw std::runtime_error("flex output aliases immutable baseline");\n if(!selected',1)
s=s.replace('auto result=baseline;','result=baseline;',1)
s=s.replace('}return result;','}',1)
s+='\n// Value-returning compatibility entry point. The Into form reuses caller storage.\ninline VvdGeometryCandidate deformModelFlexesCandidate(const VvdGeometryCandidate&baseline,const std::vector<MdlMeshFlexCandidate>&flexes,const std::vector<float>&current,const std::vector<float>&previous,float scale,const std::function<bool(uint32_t,uint32_t)>&selected,ModelFlexWorkProfileCandidate*profile=nullptr){VvdGeometryCandidate result;deformModelFlexesIntoCandidate(baseline,flexes,current,previous,scale,selected,result,profile);return result;}\n'
s=s.replace('// Returns a fresh geometry copy; baseline assets are never accumulated in place.','// Resets caller-owned output from baseline before accumulating. Output may be\n// partial on failure; do not publish it. A later call resets it before reuse.')
p.write_text(s,encoding='utf-8')
