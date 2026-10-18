from pathlib import Path
import hashlib,subprocess,uuid,json
binary=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll');before=hashlib.sha256(binary.read_bytes()).hexdigest();assert before=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
network=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\networksystem.dll');network_before=hashlib.sha256(network.read_bytes()).hexdigest();assert network_before=='edfe4d1c65d8b52818b0f8e575678a9434c0a5c69be2b1c625484a967df06866'
oracle=Path('outputs')/('network-drift-correction-'+uuid.uuid4().hex+'.bin');logs=[]
for arch,marker in [('x86','original_x86_network_drift_correction=65536'),('x64','x64_network_drift_correction_exact=65536')]:
 r=subprocess.run([str(Path('work')/('probe_network_drift_correction_'+arch+'.exe')),str(oracle)],capture_output=True,text=True);assert r.returncode==0,r.stdout+r.stderr;assert marker in r.stdout;logs.append(r.stdout.strip())
assert oracle.stat().st_size==65536*43*4;assert hashlib.sha256(binary.read_bytes()).hexdigest()==before
assert hashlib.sha256(network.read_bytes()).hexdigest()==network_before
report=dict(network_sha256=network_before,cases=65536,binary_sha256=before,installation_unchanged=True,oracle=str(oracle),results=logs,limits='Full original engine correction routine and copied original CNetworkSystem getter, controlled object/configuration, signed states -1..3 and raw network float. Default masked FP, logging disabled, supplied demo predicate. Real object lifecycle, exception flags and live playback unverified.')
Path('outputs/sfm64-network-drift-correction-native-validation.json').write_text(json.dumps(report,indent=2));print('\n'.join(logs))
