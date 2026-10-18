from pathlib import Path
from pe_image import Image
import struct,json
im=Image(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\sdktools\python\2.7\win32\Lib\site-packages\vs\valve_pyd.pyd'))
report={hex(a):{'bits':hex(im.word(a)),'float':struct.unpack('<f',struct.pack('<I',im.word(a)))[0]} for a in [0x101e63e0,0x101e6408,0x10385bdc,0x10283860]}
Path('outputs/sfm64-native-rate-conversion-constants.json').write_text(json.dumps(report,indent=2));print(report)
s=Path('work/probe_native_frame_ticks_x86.cpp').read_text()
s=s[:s.index('using Native=')]+'''using Native=void(__thiscall*)(uint32_t*,float);auto native=reinterpret_cast<Native>(mapped+0x1c2a00);std::ofstream output(argv[1],std::ios::binary);size_t cases=0;
 for(float fps:{0.f,1.f,12.f,23.976f,24.f,25.f,29.97f,30.f,48.f,50.f,59.94f,60.f,90.f,120.f,144.f,240.f,1000.f,65535.f,24.000001f,23.999999f,29.97003f,59.94006f,1.5f,2.5f,30.1f,30.4f,30.5f,30.6f,30.9f}){uint32_t rate=0;native(&rate,fps);output.write(reinterpret_cast<const char*>(&fps),4);output.write(reinterpret_cast<const char*>(&rate),4);++cases;}output.close();if(!output)throw std::runtime_error("oracle write");VirtualFree(mapped,0,MEM_RELEASE);mapped=nullptr;std::cout<<"native float rate oracle passed cases="<<cases<<" DLL_entrypoint_called=0\\n";return 0;}catch(const std::exception&e){if(mapped)VirtualFree(mapped,0,MEM_RELEASE);std::cerr<<e.what()<<'\\n';return 1;}}
'''
Path('work/probe_native_rate_x86.cpp').write_text(s)
b=Path('work/build_frame_ticks_oracle.cmd').read_text().replace('frame_ticks','rate').replace('frame-ticks','rate')
Path('work/build_rate_oracle.cmd').write_text(b)
