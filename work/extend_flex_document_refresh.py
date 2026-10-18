from pathlib import Path
p=Path('work/test_document_scene_refresh.cpp');s=p.read_text(encoding='utf-8')
s=s.replace('#include "gameinfo_mounts_candidate.h"','#include "gameinfo_mounts_candidate.h"\n#include "retained_static_flex_provider_candidate.h"',1)
s=s.replace('int main(){try{','int main(int argc,char**argv){try{bool flex=argc==2&&std::string(argv[1])=="--static-flex";if(argc>1&&!flex)throw std::runtime_error("invalid refresh test option");',1)
needle='RetainedSessionSceneCandidate renderer(Graph(doc.graph()),2237,mounts,4,true,true);'
assert s.count(needle)==1
s=s.replace(needle,needle+'if(flex){auto provider=std::make_shared<RetainedStaticFlexProviderCandidate>(mounts);renderer.setBorrowedDeformationCandidate([provider](const Graph&graph,const SessionRenderModelCandidate&instance,const SfmModelGeometryCandidate&model)->const VvdGeometryCandidate&{return provider->sampleBorrowed(graph,instance,model);});}',1)
s=s.replace('std::cout<<"document scene refresh passed','std::cout<<"static_flex="<<flex<<" document scene refresh passed',1)
p.write_text(s,encoding='utf-8')
