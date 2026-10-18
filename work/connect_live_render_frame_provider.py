from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text().replace('#include "geometry_deformation_worker_pool.h"','#include "geometry_deformation_worker_pool.h"\n#include "render_frame_provider_candidate.h"')
s=s.replace('const std::function<size_t(size_t)>&selectNextFrame={}){','const std::function<size_t(size_t)>&selectNextFrame={},const RenderFrameProviderCandidate&frameProvider={}){',1)
s=s.replace('size_t frameCount=1;', 'size_t frameCount=1;bool hasFrameInputs=sequencePathSize||bool(frameProvider.sample);if(frameProvider.sample&&sequencePathSize)throw std::runtime_error("conflicting live and file frame providers");')
s=s.replace('if(sequencePathSize)', 'if(hasFrameInputs)')
s=s.replace('sequencePathSize&&prefetchFrames', 'hasFrameInputs&&prefetchFrames')
s=s.replace('sequencePathSize&&asyncOutput', 'hasFrameInputs&&asyncOutput')
s=s.replace('auto outputPath=outputDirectory/(sequencePathSize?', 'auto outputPath=outputDirectory/(hasFrameInputs?')
marker='\n struct PreparedDrawFrameCandidate'
assert marker in s
s=s.replace(marker,'\n if(frameProvider.sample&&frameProvider.count!=frameCount)throw std::runtime_error("live frame count mismatch");'+marker)
old='auto directory=std::filesystem::path(sequencePath)/("frame-"+std::to_string(frame));checkSingleModelSequenceManifestCandidate(directory,mount.parent_path(),base);auto pose=readGeometryPoseFileCandidate((directory/"model-0-pose.bin").string(),model.bones.size());'
assert old in s
s=s.replace(old,'auto directory=std::filesystem::path(sequencePath)/("frame-"+std::to_string(frame));RenderFrameStateCandidate state;if(frameProvider.sample)state=frameProvider.sample(frame,model.bones);else{checkSingleModelSequenceManifestCandidate(directory,mount.parent_path(),base);state.locals=readGeometryPoseFileCandidate((directory/"model-0-pose.bin").string(),model.bones.size());state.placement=readRenderCameraFileCandidate((directory/"model-0-placement.bin").string());state.camera=readRenderCameraFileCandidate((directory/"camera.bin").string());}auto pose=std::move(state.locals);')
s=s.replace('auto placement=readRenderCameraFileCandidate((directory/"model-0-placement.bin").string());','auto placement=state.placement;')
s=s.replace('result.camera=readRenderCameraFileCandidate((directory/"camera.bin").string());','result.camera=state.camera;')
p.write_text(s)
p=Path('work/vulkan_graphics_pipeline_candidate.h');s=p.read_text().replace('const std::function<size_t(size_t)>&selectNextFrame={}){','const std::function<size_t(size_t)>&selectNextFrame={},const RenderFrameProviderCandidate&frameProvider={}){',1).replace('base,presentImage,selectNextFrame);','base,presentImage,selectNextFrame,frameProvider);');p.write_text(s)
