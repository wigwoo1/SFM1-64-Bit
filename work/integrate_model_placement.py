from pathlib import Path
p = Path('work/vulkan_model_draw_candidate.h')
s = p.read_text()
assert 'external_model_matrix_loaded' not in s
s = s.replace('#include "render_camera_file_candidate.h"', '#include "render_camera_file_candidate.h"\n#include "camera_rigid_inverse_candidate.h"')
old = 'char blendModeText[8]{};'
new = '''char modelMatrixPath[32768]{};auto modelMatrixSize=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_MODEL_MATRIX",modelMatrixPath,sizeof(modelMatrixPath));if(modelMatrixSize){if(modelMatrixSize>=sizeof(modelMatrixPath))throw std::runtime_error("model matrix path length");auto matrix=readRenderCameraFileCandidate(modelMatrixPath);if(matrix.rows[12]!=0||matrix.rows[13]!=0||matrix.rows[14]!=0||matrix.rows[15]!=1)throw std::runtime_error("model placement affine row");std::array<std::array<float,4>,3>rigid;for(size_t r=0;r<3;++r)for(size_t c=0;c<4;++c)rigid[r][c]=matrix.rows[r*4+c];cameraRigidInverseCandidate(rigid);for(auto&v:vertices){auto position=matrix.clip(v.position);v.position={position[0],position[1],position[2]};auto normal=v.normal;for(size_t r=0;r<3;++r)v.normal[r]=matrix.rows[r*4]*normal[0]+matrix.rows[r*4+1]*normal[1]+matrix.rows[r*4+2]*normal[2];}std::cout<<"external_model_matrix_loaded=1 vertices="<<vertices.size()<<"\\n";}
 char blendModeText[8]{};'''
assert old in s
s = s.replace(old, new, 1)
p.write_text(s)
