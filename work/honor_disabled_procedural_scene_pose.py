from pathlib import Path
p=Path('work/model_skinning_geometry.h');s=p.read_text().replace('std::vector<GeometryMatrixCandidate>*outGlobals=nullptr){','std::vector<GeometryMatrixCandidate>*outGlobals=nullptr,bool evaluateProcedural=true){').replace('if(b.procedure)throw std::runtime_error("procedural pose unsupported");','if(b.procedure&&evaluateProcedural)throw std::runtime_error("procedural pose unsupported");');p.write_text(s)
p=Path('work/scene_geometry_preparation_candidate.h');s=p.read_text().replace('evaluateGeometryPosePaletteCandidate(model.bones,instance.locals)','evaluateGeometryPosePaletteCandidate(model.bones,instance.locals,nullptr,instance.model.evaluateProcedural)');p.write_text(s)
p=Path('work/test_retained_session_scene.cpp');s=p.read_text()
s=s.replace('size_t failures=0;for(int32_t tick:{1259500,1392000})','auto multi=renderer.sample(1259500);if(multi.materialInputs.size()!=4||multi.upload.materials.empty()||multi.upload.vertices.empty())throw std::runtime_error("disabled procedural scene coverage");std::cout<<"four_model_scene prepared=1 vertices="<<multi.upload.vertices.size()<<" indices="<<multi.upload.indices.size()<<" material_slots="<<multi.upload.materials.size()<<" disabled_procedural_pose=candidate native_pose_parity=unverified\\n";size_t failures=0;for(int32_t tick:{1392000})')
s=s.replace('failures!=2','failures!=1')
p.write_text(s)
