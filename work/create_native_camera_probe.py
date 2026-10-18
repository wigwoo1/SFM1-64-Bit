from pathlib import Path
s=Path('work/probe_native_curve_components.cpp').read_text()
start=s.index('int main(');end=s.index(' if(argc!=2)',start)
prefix=s[start:end].replace('game\\\\bin\\\\tools\\\\ifm.dll','game\\\\sdktools\\\\python\\\\2.7\\\\win32\\\\Lib\\\\site-packages\\\\vs\\\\valve_pyd.pyd')
headers='#define NOMINMAX\n#include <windows.h>\n#include <fstream>\n#include <vector>\n#include <array>\n#include <iostream>\n#include <cstring>\n#include <stdexcept>\n#include "recovered_camera_projection_candidate.h"\n'
body=r'''
 if(argc!=2)throw std::runtime_error("fresh oracle output required");if(std::ifstream(argv[1]).good())throw std::runtime_error("oracle exists");std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("oracle output");using Native=void(__cdecl*)(float*,float,float,float,float);auto native=reinterpret_cast<Native>(0x101BD050);unsigned count=0,mismatches=0;double maxError=0;
 for(float fov:{1.f,25.2f,60.f,90.f,120.f,170.f})for(float aspect:{.5f,1.f,4.f/3.f,16.f/9.f,2.f})for(auto planes:{std::array<float,2>{3,28377.9f},std::array<float,2>{.1f,100.f}}){std::array<float,16>matrix{};native(matrix.data(),planes[0],planes[1],fov,aspect);auto candidate=recoveredCameraProjectionCandidate(planes[0],planes[1],fov,aspect,false);std::array<float,4>args{planes[0],planes[1],fov,aspect};output.write(reinterpret_cast<const char*>(args.data()),16);output.write(reinterpret_cast<const char*>(matrix.data()),64);for(size_t n=0;n<16;++n){if(!std::isfinite(matrix[n]))throw std::runtime_error("native nonfinite");maxError=std::max(maxError,std::abs(double(matrix[n])-candidate.rows[n]));}if(std::memcmp(matrix.data(),candidate.rows.data(),64))++mismatches;++count;}
 output.close();if(!output)throw std::runtime_error("oracle write");std::cout<<"native_projection_cases="<<count<<" bitwise_mismatches="<<mismatches<<" max_absolute_error="<<maxError<<" DLL_entrypoint_not_called=1\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
'''
Path('work/probe_native_camera_projection_x86.cpp').write_text(headers+prefix+body)
