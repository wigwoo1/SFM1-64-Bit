from pathlib import Path
def edit(name,changes):
 p=Path('work')/name;s=p.read_text(encoding='utf-8')
 for old,new in changes:
  assert s.count(old)==1,(name,old,s.count(old));s=s.replace(old,new,1)
 p.write_text(s,encoding='utf-8')
edit('scene_geometry_preparation_candidate.h',[
 ('inline std::vector<SceneGeometryModelCandidate>','// Borrowed geometry must remain valid until synchronous workers.deform returns.\nusing SceneBorrowedDeformationCandidate=std::function<const VvdGeometryCandidate&(const SessionRenderModelCandidate&,const SfmModelGeometryCandidate&)>;\ninline std::vector<SceneGeometryModelCandidate>'),
 ('const SceneModelDeformationCandidate&deform={})','const SceneModelDeformationCandidate&deform={},const SceneBorrowedDeformationCandidate&borrowed={})'),
 (' if(!asset)',' if(deform&&borrowed)throw std::runtime_error("conflicting scene deformation providers");\n if(!asset)'),
 ('if(deform){deformed=deform(instance.model,model);if(deformed.checksum','if(deform||borrowed){if(borrowed)geometry=&borrowed(instance.model,model);else{deformed=deform(instance.model,model);geometry=&deformed;}const auto&checked=*geometry;if(checked.checksum'),
 ('deformed.lod!=','checked.lod!='),('deformed.vertices.size()!=','checked.vertices.size()!='),('deformed.tangents.size()!=','checked.tangents.size()!='),
 ('throw std::runtime_error("scene deformation changed geometry addressing");geometry=&deformed;}','throw std::runtime_error("scene deformation changed geometry addressing");}')])
edit('prepared_session_scene_candidate.h',[
 ('const SceneModelDeformationCandidate&deform={})','const SceneModelDeformationCandidate&deform={},const SceneBorrowedDeformationCandidate&borrowed={})'),
 ('prepareSceneGeometryCandidate(poses,asset,workers,deform)','prepareSceneGeometryCandidate(poses,asset,workers,deform,borrowed)')])
edit('retained_session_scene_candidate.h',[
 ('class RetainedSessionSceneCandidate','using RetainedBorrowedDeformationCandidate=std::function<const VvdGeometryCandidate&(const Graph&,const SessionRenderModelCandidate&,const SfmModelGeometryCandidate&)>;\nclass RetainedSessionSceneCandidate'),
 (' RetainedModelDeformationCandidate deformation_;',' RetainedModelDeformationCandidate deformation_;RetainedBorrowedDeformationCandidate borrowed_;'),
 ('deformation_=std::move(provider);}','deformation_=std::move(provider);borrowed_={};}\n void setBorrowedDeformationCandidate(RetainedBorrowedDeformationCandidate provider){borrowed_=std::move(provider);deformation_={};}'),
 ('auto result=prepareSessionSceneCandidate','SceneBorrowedDeformationCandidate borrowed;if(borrowed_)borrowed=[this](const SessionRenderModelCandidate&instance,const SfmModelGeometryCandidate&model)->const VvdGeometryCandidate&{return borrowed_(timeline_.graph(),instance,model);};auto result=prepareSessionSceneCandidate'),
 ('allowScale_,allowNameMapping_,deform);','allowScale_,allowNameMapping_,deform,borrowed);')])
edit('retained_static_flex_provider_candidate.h',[
 (' std::vector<std::filesystem::path>mounts_;',' std::map<uint32_t,VvdGeometryCandidate>outputs_;\n std::vector<std::filesystem::path>mounts_;'),
 (' VvdGeometryCandidate sample(',' void sampleInto('),
 ('const SfmModelGeometryCandidate&model,StaticFlexStageProfileCandidate*profile=nullptr){','const SfmModelGeometryCandidate&model,VvdGeometryCandidate&result,StaticFlexStageProfileCandidate*profile=nullptr){'),
 ('auto result=deformModelFlexesCandidate(','deformModelFlexesIntoCandidate('),
 ('return body.at(b)==m;},profile?','return body.at(b)==m;},result,profile?'),
 (';return result;',';'),
 ('\n};','\n VvdGeometryCandidate sample(const Graph&graph,const SessionRenderModelCandidate&instance,const SfmModelGeometryCandidate&model,StaticFlexStageProfileCandidate*profile=nullptr){VvdGeometryCandidate result;sampleInto(graph,instance,model,result,profile);return result;}\n const VvdGeometryCandidate&sampleBorrowed(const Graph&graph,const SessionRenderModelCandidate&instance,const SfmModelGeometryCandidate&model){auto&result=outputs_[instance.id];sampleInto(graph,instance,model,result);return result;}\n};')])
