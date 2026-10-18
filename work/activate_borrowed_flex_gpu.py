from pathlib import Path
import json
for name in ('static-flex','four-model-static-flex'):
 source=Path(f'outputs/sfm64-{name}-gpu-validation.json')
 target=Path(f'outputs/sfm64-{name}-before-borrowed-output.json')
 assert not target.exists(), 'do not overwrite prior reference'
 target.write_bytes(source.read_bytes())
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text(encoding='utf-8')
old='frozenSceneOwner->setModelDeformationCandidate([provider](const Graph&graph,const SessionRenderModelCandidate&instance,const SfmModelGeometryCandidate&value){return provider->sample(graph,instance,value);});'
assert s.count(old)==1
s=s.replace(old,'frozenSceneOwner->setBorrowedDeformationCandidate([provider](const Graph&graph,const SessionRenderModelCandidate&instance,const SfmModelGeometryCandidate&value)->const VvdGeometryCandidate&{return provider->sampleBorrowed(graph,instance,value);});',1)
p.write_text(s,encoding='utf-8')
