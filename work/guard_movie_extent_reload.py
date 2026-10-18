from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text();s=s.replace('#include "session_render_root_candidate.h"','#include "session_render_root_candidate.h"\n#include "session_movie_dimensions_candidate.h"',1)
old='auto root=sessionRenderRootCandidate(graph,explicitSceneRoot);frozenSceneOwner->replaceSession(std::move(graph),root);'
new='auto root=sessionRenderRootCandidate(graph,explicitSceneRoot);if(GetEnvironmentVariableA("SFM64_DIAGNOSTIC_SCENE_OUTPUT",nullptr,0)){auto dimensions=sessionMovieDimensionsCandidate(graph,root);if(dimensions.width!=outputWidth||dimensions.height!=outputHeight)throw std::runtime_error("movie dimensions changed; target rebuild required");}frozenSceneOwner->replaceSession(std::move(graph),root);'
assert old in s;s=s.replace(old,new,1);p.write_text(s)
