from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
s=s.replace('#include "camera_rigid_inverse_candidate.h"','#include "camera_rigid_inverse_candidate.h"\n#include "geometry_pose_file_candidate.h"')
old='for(auto&b:model.bones)locals.push_back(b.local);auto palette='
new='for(auto&b:model.bones)locals.push_back(b.local);char posePath[32768]{};auto posePathSize=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_BONE_POSE",posePath,sizeof(posePath));if(posePathSize){if(posePathSize>=sizeof(posePath))throw std::runtime_error("pose path length");locals=readGeometryPoseFileCandidate(posePath,model.bones.size());std::cout<<"external_bone_pose_loaded=1 bones="<<locals.size()<<"\\n";}auto palette='
assert old in s;s=s.replace(old,new,1);p.write_text(s)
