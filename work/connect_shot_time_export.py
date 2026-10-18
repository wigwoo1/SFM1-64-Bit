from pathlib import Path
p=Path('work/export_shot_render_inputs_x64.cpp')
s=p.read_text()
s=s.replace('#include "film_scene_render_binding_candidate.h"','#include "film_scene_render_binding_candidate.h"\n#include "film_shot_time_candidate.h"')
s=s.replace('if(argc!=5)throw std::runtime_error("session film-clip-id content-root fresh-output-directory required");','if(argc!=5&&argc!=6)throw std::runtime_error("session film-clip-or-root-id content-root fresh-output-directory [root-parent-tick] required");')
s=s.replace('auto graph=loadGraph(argv[1]);auto binding=', 'auto graph=loadGraph(argv[1]);int32_t selectedLocalTick=0;if(argc==6){size_t tickUsed=0;std::string tickText=argv[5];auto tick=std::stoll(tickText,&tickUsed);if(tickUsed!=tickText.size()||tick<INT32_MIN||tick>INT32_MAX)throw std::runtime_error("timeline tick");auto selected=selectFilmShotCandidate(graph,uint32_t(clip),int32_t(tick));clip=selected.clip;selectedLocalTick=selected.localTick;}auto binding=')
s=s.replace('for(size_t n=0;n<models.size();++n)', 'if(argc==6)manifest<<"root_parent_tick="<<argv[5]<<" selected_local_tick="<<selectedLocalTick<<" selection_policy=candidate_half_open animation_sampled=no\\n";\n for(size_t n=0;n<models.size();++n)')
p.write_text(s)
