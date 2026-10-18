from pathlib import Path
p=Path('work/test_retained_session_scene.cpp');s=p.read_text();needle='size_t failures=0;'
insert='''auto enabledGraph=loadGraph("work/roundtrip-11.dmx");size_t changed=0;for(auto&attribute:enabledGraph.elements.at(20064).attributes)if(enabledGraph.strings.at(attribute.name)=="evaluateProceduralBones"){attribute.values.at(0).wire.at(0)=1;++changed;}if(changed!=1)throw std::runtime_error("procedural flag test coverage");RetainedSessionSceneCandidate enabledRenderer(std::move(enabledGraph),2237,mounts,4,true,true);bool proceduralRejected=false;try{enabledRenderer.sample(1259500);}catch(const std::runtime_error&e){proceduralRejected=std::string(e.what())=="procedural pose unsupported";}if(!proceduralRejected)throw std::runtime_error("enabled procedural simulation accepted");std::cout<<"enabled_procedural_simulation_rejected=1\\n";'''
if s.count(needle)!=1:raise RuntimeError('failure marker')
p.write_text(s.replace(needle,insert+needle))
