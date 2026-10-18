from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text().replace('#include <future>','#include <future>\n#include "geometry_deformation_worker_pool.h"')
s=s.replace('auto deformed=deformGeometryBatchCandidate(model.geometry,palette,4);','GeometryDeformationWorkerPoolCandidate deformationPool(4);auto deformed=deformationPool.deform(model.geometry,palette);')
s=s.replace('auto frameVertices=deformGeometryBatchCandidate(model.geometry,framePalette,4);','auto frameVertices=deformationPool.deform(model.geometry,framePalette);')
s=s.replace(' max_prepared_ahead=1 native_full_parity=unverified',' max_prepared_ahead=1 retained_geometry_workers="<<deformationPool.workerCount()<<" native_full_parity=unverified')
p.write_text(s)
p=Path('work/test_parallel_geometry_deformation_x64.cpp');s=p.read_text().replace('#include "parallel_geometry_deformation.h"','#include "geometry_deformation_worker_pool.h"')
s=s.replace('auto start=std::chrono::steady_clock::now();for(int iteration=', 'GeometryDeformationWorkerPoolCandidate pool(workers);auto start=std::chrono::steady_clock::now();for(int iteration=')
s=s.replace('auto result=deformGeometryBatchCandidate(geometry,palette,workers);','auto result=pool.deform(geometry,palette);')
s=s.replace('try{deformGeometryBatchCandidate(bad,palette,workers);}', 'try{GeometryDeformationWorkerPoolCandidate pool(workers?workers:1);pool.deform(bad,palette);auto recovered=pool.deform(geometry,palette);if(recovered.size()!=reference.size())throw std::runtime_error("recovery count");}')
# Test recovery after the deliberately thrown first-vertex error on the same pool.
s=s.replace('++rejected;}','GeometryDeformationWorkerPoolCandidate recovery(workers?workers:1);try{recovery.deform(bad,palette);}catch(const std::runtime_error&){}auto good=recovery.deform(geometry,palette);for(size_t n=0;n<good.size();++n)if(!equal(good[n],reference[n]))throw std::runtime_error("pool recovery mismatch");++rejected;}',1)
s=s.replace('x64 parallel deformation byte-exact','x64 retained geometry pool byte-exact')
Path('work/test_geometry_worker_pool_x64.cpp').write_text(s)
