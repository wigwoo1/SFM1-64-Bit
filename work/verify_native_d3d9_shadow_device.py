from pathlib import Path
import subprocess,uuid,json,hashlib,sys
root=Path(__file__).resolve().parent.parent
directory=root/'outputs'/('native-d3d9-'+uuid.uuid4().hex);directory.mkdir();capture=directory/'frame.bgra'
indexed=len(sys.argv)==2 and sys.argv[1]=='indexed'
if len(sys.argv)>1 and not indexed:raise ValueError('optional argument: indexed')
arguments=[str(root/'work/probe_native_d3d9_shadow_device.exe'),str(capture)]+(['indexed'] if indexed else [])
run=subprocess.run(arguments,capture_output=True,text=True,timeout=60);log=run.stdout+run.stderr;(directory/'run.txt').write_text(log);print(log.strip())
unavailable=run.returncode!=0 and 'no exposed D3D9 HAL adapter supports' in log
if run.returncode and not unavailable:raise RuntimeError(log)
shader_verified='installed_shader_creation_binding_readback=passed' in log
preflight_verified='installed_shader_CPU_preflight=passed constant_uploads=7' in log
factory_verified='factory_lifecycle_setup=passed' in log and 'factory_lifecycle_release_clear_cleanup=passed' in log
creation_policy_verified='device_creation_attempt_policy=passed' in log
if not unavailable and not creation_policy_verified:raise RuntimeError('device creation policy evidence missing')
if not factory_verified:raise RuntimeError('D3D9 factory lifecycle evidence missing')
draw_verified='installed_source_shader_draw=passed' in log
buffer_draw_verified=('owned_index_buffer_indexed_adapter=passed' if indexed else 'owned_vertex_buffer_nonindexed_adapter=passed') in log
if not unavailable and not buffer_draw_verified:raise RuntimeError('owned vertex buffer draw adapter evidence missing')
constants_verified='shader_constant_command_upload_readback=passed' in log
reuse_verified='retained_constant_plan_repeat_uploads=0' in log
metadata_verified='lighting_metadata_device_upload_readback_reuse=passed restored_original_registers=1' in log
bundle_verified='combined_lighting_bundle_device_upload_readback_reuse=passed invalidation=passed' in log
viewport_verified='viewport_adapter_device_readback_reuse=passed' in log
constant_runs_verified='vertex_constant_run_device_readback_reuse_restore=passed' in log
pixel_color_verified='pixel_color_device_readback_reuse_restore=passed' in log
if not unavailable and not pixel_color_verified:raise RuntimeError('pixel color device evidence missing')
texture_verified='vertex_texture_device_binding_sampler_reuse_unbind=passed' in log
general_texture_verified='general_texture_binding_device_sampler_readback_reuse_unbind=passed' in log
if not unavailable and not general_texture_verified:raise RuntimeError('general texture binding device evidence missing')
if not unavailable and not texture_verified:raise RuntimeError('vertex texture device evidence missing')
if not unavailable and not constant_runs_verified:raise RuntimeError('vertex constant run device evidence missing')
if not unavailable and not viewport_verified:raise RuntimeError('viewport adapter readback/reuse evidence missing')
if not preflight_verified:raise RuntimeError('installed shader CPU preflight evidence missing')
if not unavailable and not shader_verified:raise RuntimeError('shader binding success evidence missing')
if not unavailable and not draw_verified:raise RuntimeError('installed shader draw evidence missing')
if not unavailable and not constants_verified:raise RuntimeError('shader constant register readback evidence missing')
if not unavailable and not reuse_verified:raise RuntimeError('retained constant reuse evidence missing')
if not unavailable and not metadata_verified:raise RuntimeError('lighting metadata device evidence missing')
if not unavailable and not bundle_verified:raise RuntimeError('combined lighting bundle device evidence missing')
report={'status':'hardware_device_unavailable' if unavailable else 'passed','x64_executable':True,'d3d9_hal_device_verified':not unavailable,'device_state_readback_verified':not unavailable,'gpu_capture_verified':not unavailable,'installed_shader_CPU_preflight_verified':preflight_verified,'installed_shader_creation_binding_verified':shader_verified,'installed_source_shader_draw_verified':draw_verified,'output_directory':str(directory),'runtime_output':log,'limits':'Standalone native D3D9 diagnostic now retains installed UnlitGeneric shaders during its triangle draw. Explicit identity model/view transforms and modulation/UV constants are diagnostic values, not recovered Source scene upload policy. Current HAL unavailability leaves creation/binding/declaration/uploads/draw/readback unverified. Material combo selection, scene geometry and complete native renderer parity remain unfinished.'}
if not unavailable:report.update(gpu_frame_bytes=capture.stat().st_size,gpu_frame_sha256=hashlib.sha256(capture.read_bytes()).hexdigest())
report['shader_constant_command_upload_readback_verified']=constants_verified
report['retained_constant_GPU_reuse_verified']=reuse_verified
report['lighting_metadata_device_upload_readback_reuse_verified']=metadata_verified
report['combined_lighting_bundle_device_verified']=bundle_verified
report['viewport_adapter_device_readback_reuse_verified']=viewport_verified
report['vertex_constant_run_device_readback_reuse_restore_verified']=constant_runs_verified
report['pixel_color_device_readback_reuse_restore_verified']=pixel_color_verified
report['d3d9_factory_lifecycle_verified']=factory_verified
report['device_creation_attempt_policy_verified']=creation_policy_verified
report['vertex_texture_device_binding_sampler_reuse_unbind_verified']=texture_verified
report['general_texture_binding_device_sampler_readback_reuse_unbind_verified']=general_texture_verified
report['owned_vertex_buffer_nonindexed_adapter_verified']=buffer_draw_verified
if indexed:
 report['owned_vertex_buffer_nonindexed_adapter_verified']=False
 report['owned_index_buffer_indexed_adapter_verified']=buffer_draw_verified
report['draw_path']='indexed' if indexed else 'nonindexed'
report['executable_sha256']=hashlib.sha256((root/'work/probe_native_d3d9_shadow_device.exe').read_bytes()).hexdigest()
(root/'outputs'/('sfm64-native-d3d9-indexed-device-validation.json' if indexed else 'sfm64-native-d3d9-shadow-device-validation.json')).write_text(json.dumps(report,indent=2)+'\n')

