from pathlib import Path
s=Path('work/probe_drift_correction.cpp').read_text()
s=s.replace('array<uint32_t,41>','array<uint32_t,43>')
s=s.replace('clockDriftCorrectionCandidate(v,r[16],r[17],r[18],r[19],r[20],r[21]!=0,r[22]!=0,r[23]!=0)', 'clockDriftCorrectionWithNetworkStateCandidate(v,r[16],r[17],r[18],r[19],r[20],int32_t(r[41]),r[42],int32_t(r[21]),r[22]!=0,r[23]!=0)')
at=s.index(' std::array<uint32_t,16>low')
s=s[:at]+''' // Execute original provider getter bytes in a private executable allocation.
 std::ifstream networkInput(R"(D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\networksystem.dll)",std::ios::binary);
 std::vector<uint8_t> networkFile((std::istreambuf_iterator<char>(networkInput)),{});
 if(networkFile.size()<sizeof(IMAGE_DOS_HEADER))throw std::runtime_error("network DOS bounds");
 auto ndos=reinterpret_cast<const IMAGE_DOS_HEADER*>(networkFile.data());
 if(ndos->e_magic!=IMAGE_DOS_SIGNATURE||ndos->e_lfanew<0||size_t(ndos->e_lfanew)>networkFile.size()-sizeof(IMAGE_NT_HEADERS32))throw std::runtime_error("network PE bounds");
 auto nnt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(networkFile.data()+ndos->e_lfanew);
 if(nnt->Signature!=IMAGE_NT_SIGNATURE||nnt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC)throw std::runtime_error("network PE identity");
 auto nsections=IMAGE_FIRST_SECTION(nnt);size_t getterOffset=0;
 if(reinterpret_cast<const uint8_t*>(nsections)+size_t(nnt->FileHeader.NumberOfSections)*sizeof(IMAGE_SECTION_HEADER)>networkFile.data()+networkFile.size())throw std::runtime_error("network section bounds");
 for(unsigned j=0;j<nnt->FileHeader.NumberOfSections;++j){auto&sec=nsections[j];if(sec.VirtualAddress<=0x96b0&&0x96b4<=sec.VirtualAddress+sec.SizeOfRawData)getterOffset=sec.PointerToRawData+0x96b0-sec.VirtualAddress;}
 const uint8_t expectedGetter[]={0xd9,0x41,0x30,0xc3};
 if(!getterOffset||getterOffset+4>networkFile.size()||std::memcmp(networkFile.data()+getterOffset,expectedGetter,4))throw std::runtime_error("network getter identity");
 auto getter=VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
 if(!getter)throw std::runtime_error("getter allocation");
 std::unique_ptr<void,void(*)(void*)> getterOwner(getter,[](void*p){VirtualFree(p,0,MEM_RELEASE);});
 std::memcpy(getter,networkFile.data()+getterOffset,4);DWORD getterOld;
 if(!VirtualProtect(getter,4096,PAGE_EXECUTE_READ,&getterOld))throw std::runtime_error("getter protection");
 FlushInstructionCache(GetCurrentProcess(),getter,4);
 std::array<uintptr_t,44>networkTable{};networkTable[43]=reinterpret_cast<uintptr_t>(getter);
 std::array<uint32_t,16>networkObject{};networkObject[0]=uint32_t(reinterpret_cast<uintptr_t>(networkTable.data()));
'''+s[at:]
s=s.replace('uintptr_t(0x10643ed8)})','uintptr_t(0x10643ed8),uintptr_t(0x106941c4)})')
s=s.replace(' using Routine=', ' *reinterpret_cast<uintptr_t*>(0x106941c4)=reinterpret_cast<uintptr_t>(networkObject.data());\n using Routine=')
s=s.replace('r[23]=(i>>2)&1;', 'r[23]=(i>>2)&1;r[41]=uint32_t(int32_t((i>>3)%5)-1);seed=seed*1664525u+1013904223u;r[42]=i<512?bounds[(i>>3)%8]:seed;')
s=s.replace(' low[11]=r[18];',' *reinterpret_cast<uint32_t*>(0x1060d4ec)=r[41];networkObject[12]=r[42];\n low[11]=r[18];')
s=s.replace('drift_correction','network_drift_correction')
# Preserve actual header filename after renaming probe markers.
s=s.replace('"clock_network_drift_correction_candidate.h"','"clock_drift_correction_candidate.h"')
Path('work/probe_network_drift_correction.cpp').write_text(s)
Path('work/build_network_drift_probe.cmd').write_text(Path('work/build_drift_correction_probe.cmd').read_text().replace('probe_drift_correction','probe_network_drift_correction'))
v=Path('work/verify_drift_correction_native.py').read_text().replace('probe_drift_correction','probe_network_drift_correction').replace('original_x86_drift_correction','original_x86_network_drift_correction').replace('x64_drift_correction_exact','x64_network_drift_correction_exact').replace('65536*41*4','65536*43*4').replace("'drift-correction-'","'network-drift-correction-'").replace('sfm64-drift-correction-native-validation','sfm64-network-drift-correction-native-validation')
v=v.replace("oracle=Path", "network=Path(r'D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\networksystem.dll');network_before=hashlib.sha256(network.read_bytes()).hexdigest();assert network_before=='edfe4d1c65d8b52818b0f8e575678a9434c0a5c69be2b1c625484a967df06866'\noracle=Path")
v=v.replace('report=dict(', 'assert hashlib.sha256(network.read_bytes()).hexdigest()==network_before\nreport=dict(network_sha256=network_before,')
a=v.index("limits='");b=v.index("')",a)
v=v[:a]+"limits='Full original engine correction routine and copied original CNetworkSystem getter, controlled object/configuration, signed states -1..3 and raw network float. Default masked FP, logging disabled, supplied demo predicate. Real object lifecycle, exception flags and live playback unverified."+v[b:]
Path('work/verify_network_drift_native.py').write_text(v)
