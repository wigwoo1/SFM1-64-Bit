from pathlib import Path
p=Path('work/export_session_camera_candidate_x64.cpp');s=p.read_text()
s=s.replace('if(argc!=3)', 'if(argc!=3&&argc!=4)')
s=s.replace('auto graph=loadGraph(argv[1]);','bool engineAxes=argc==4&&std::string(argv[3])=="engine";if(argc==4&&!engineAxes)throw std::runtime_error("camera mode");auto graph=loadGraph(argv[1]);')
s=s.replace('auto camera=sessionCameraViewProjectionCandidate(graph,n,512,512);','if(engineAxes)local=engineCameraBasisCandidate(local);auto camera=sessionCameraViewProjectionCandidate(graph,n,512,512,engineAxes);')
s=s.replace('engine_axes=false full_native_parity=unverified','engine_axes="<<engineAxes<<" full_native_parity=unverified')
p.write_text(s)
