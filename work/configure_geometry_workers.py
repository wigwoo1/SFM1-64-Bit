from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
a='GeometryDeformationWorkerPoolCandidate deformationPool(4);'
b='''size_t geometryWorkers=4;char workerText[32]{};auto workerSize=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_GEOMETRY_WORKERS",workerText,sizeof(workerText));if(workerSize){if(workerSize>=sizeof(workerText))throw std::runtime_error("geometry worker option length");auto parsed=std::from_chars(workerText,workerText+workerSize,geometryWorkers);if(parsed.ec!=std::errc{}||parsed.ptr!=workerText+workerSize||!geometryWorkers||geometryWorkers>64)throw std::runtime_error("geometry workers must be 1..64");}GeometryDeformationWorkerPoolCandidate deformationPool(geometryWorkers);std::cout<<"geometry_worker_count="<<geometryWorkers<<" range_policy=contiguous\\n";'''
assert s.count(a)==1;s=s.replace(a,b);p.write_text(s)
