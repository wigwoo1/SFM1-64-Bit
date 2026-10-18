#include "material_uv_transform_candidate.h"
#include <iostream>
int main(){try{
 auto identity=materialUvTransformCandidate(parseVmtCandidate("\"UnlitGeneric\" { }"));if(identity!=identityMaterialUvTransformCandidate())throw std::runtime_error("default UV transform");
 auto matrix=materialUvTransformCandidate(parseVmtCandidate("\"UnlitGeneric\" { \"$basetexturetransform\" \"[1 0 0 0.003242 0 1 0 -0.015513 0 0 1 0 0 0 0 1]\" }"));if(std::abs(matrix[2]-.003242f)>1e-7f||std::abs(matrix[6]+.015513f)>1e-7f||matrix[0]!=1||matrix[5]!=1)throw std::runtime_error("DMX matrix UV transform");
 auto vmt=materialUvTransformCandidate(parseVmtCandidate("\"UnlitGeneric\" { \"$basetexturetransform\" \"center 0.5 0.5 scale 2 3 rotate 90 translate 0.1 0.2\" }"));if(std::abs(vmt[0])>1e-6f||std::abs(vmt[1]+3)>1e-6f||std::abs(vmt[4]-2)>1e-6f||std::abs(vmt[5])>1e-6f||std::abs(vmt[2]-2.1f)>1e-6f||std::abs(vmt[6]+.3f)>1e-6f)throw std::runtime_error("VMT scale/rotate/center/translation");
 std::cout<<"material UV transform passed identity=1 VMatrix_translation=1 VMT_decomposition=1\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
