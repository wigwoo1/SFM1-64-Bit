from pathlib import Path
p=Path('work/live_session_frame_provider_candidate.h');s=p.read_text();old='auto initialOverrides=readSessionMaterialOverridesCandidate(graph,firstBinding.models.front());';new='RetainedShotTimelineCandidate initialMaterialTimeline(graph,root);initialMaterialTimeline.step(ticks.front(),2,2);auto initialOverrides=readSessionMaterialOverridesCandidate(initialMaterialTimeline.graph(),firstBinding.models.front());'
if s.count(old)!=1:raise RuntimeError('initial override marker')
p.write_text(s.replace(old,new))
