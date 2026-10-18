import hashlib,json,re,subprocess
from pathlib import Path
cache=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\platform\shaders\fxc\vertexlit_and_unlit_generic_vs20.vcs')
before=hashlib.sha256(cache.read_bytes()).hexdigest()
pixelCache=cache.with_name('vertexlit_and_unlit_generic_ps20.vcs')
pixelBefore=hashlib.sha256(pixelCache.read_bytes()).hexdigest()
pixelBCache=cache.with_name('vertexlit_and_unlit_generic_ps20b.vcs')
pixelBBefore=hashlib.sha256(pixelBCache.read_bytes()).hexdigest()
result=subprocess.run(['work/test_generic_lighting_draw.exe'],capture_output=True,text=True,check=True)
assert 'lighting_draw_state_cases=160 installed_programs_present=144 absent_or_out_of_range=16 decoded_blocks=2' in result.stdout
assert 'checked_domain_rejections=32 in_range_unsafe=16' in result.stdout
assert 'installed_lighting_shader_pairs=128' in result.stdout
assert 'installed_lighting_shader2b_pairs=128 shader2b_absent=128' in result.stdout
assert 'installed_flashlight_shader_pairs=256 flashlight_absent=0' in result.stdout
assert 'generic_ambient_metadata_uploads=3 retained_repeat_uploads=0' in result.stdout
assert 'retained_lighting_constant_records=6 capacity_failure_preserves_record=1' in result.stdout
assert 'lighting_metadata_initial_uploads=2 repeat_uploads=0 force_and_failure_checks=passed' in result.stdout
assert 'light_float_upload_plans=50 repeat_uploads=0 source_bounds_checks=passed' in result.stdout
assert 'lighting_upload_bundle_float_ranges=5 metadata_ranges=2 repeated_uploads=0' in result.stdout
assert 'shader_lighting_preparation_commit=passed failed_preparation_preserves_selection=1' in result.stdout
assert 'single_draw_derived_shader_and_lighting=passed invalid_domain_preserves_selection=1' in result.stdout
assert 'parallel_draw_preparation_exact=32 workers=4 failure_propagation=passed' in result.stdout
assert 'parallel_cold_shader_and_metadata_exact=32 retry_after_failure=passed' in result.stdout
assert hashlib.sha256(pixelBCache.read_bytes()).hexdigest()==pixelBBefore
assert hashlib.sha256(pixelCache.read_bytes()).hexdigest()==pixelBefore
unresolved=[]
for line in result.stdout.splitlines():
 if line.startswith('unresolved '):
  inputs={key:int(value) for key,value in re.findall(r'(ambient|lights|compression|bones|flow|query|dynamic)=(\d+)',line)}
  assert inputs['lights']>=3 and inputs['flow']==0 and inputs['query']==2 and inputs['dynamic']>=48
  assert line.endswith('reason=VCS6 combo range');unresolved.append(inputs)
assert len(unresolved)==16 and hashlib.sha256(cache.read_bytes()).hexdigest()==before
report={'status':'passed_scoped_composition','cases':160,'programs_present':144,'out_of_range':16,'missing_in_range':0,'cache_sha256':before,'cache_unchanged':True,'unresolved_inputs':unresolved,'limits':'CPU composition using explicit static0. Sample input combinations do not establish native legal domains, hardware/query semantics, branch policy or GPU/application behavior.'}
report['checked_policy_rejections']=32
report['in_range_unsafe_combinations']=16
report['checked_resolved_states']=128
report['checked_selector_unique_indices']=48
report['installed_shader_pairs']=128
report['pixel_cache_sha256']=pixelBefore
report['pixel2b_cache_sha256']=pixelBBefore
report['installed_shader2b_pairs']=128
report['shader2b_shadow_requests_absent']=128
report['shader2b_failed_requests_preserve_selection']=True
report['installed_flashlight_shader_pairs']=256
report['flashlight_static_vertex_argument']=384
report['flashlight_static_pixel_argument']=1024
report['flashlight_pairs_absent']=0
report['ambient_metadata_bound_uploads']=3
report['retained_lighting_constant_records']=6
report['lighting_metadata_initial_uploads']=2
report['light_float_upload_plan_cases']=50
report['lighting_upload_bundle']={'float_ranges':5,'metadata_ranges':2,'repeat_uploads':0,'failed_preparation_preserves_input':True}
report['shader_lighting_preparation_preserves_selection_on_failure']=True
report['single_draw_derived_preparation_verified']=True
benchmark=re.search(r'draw_preparation_benchmark_requests=4096 trials=5 serial_median_us=(\d+) parallel4_median_us=(\d+)',result.stdout)
assert benchmark
serial_us,parallel_us=map(int,benchmark.groups());assert serial_us>0 and parallel_us>0
report['parallel_draw_preparation']={'requests':32,'workers':4,'serial_shader_and_float_words_match':True,'failure_propagation':True,'performance_gain_measured':True}
report['parallel_draw_preparation']['cold_shader_and_metadata_match']=True
report['parallel_draw_preparation']['retry_after_failure']=True
report['draw_preparation_benchmark']={'requests':4096,'trials':5,'serial_median_us':serial_us,'parallel4_median_us':parallel_us,'ratio':serial_us/parallel_us,'scope':'Warm-cache CPU preparation, fixed one/two-light requests; includes per-batch thread creation, excludes GPU uploads, shader cold decode and full application frame time.'}
report['light_float_upload_scope']='Owned stable configuration and explicitly supplied unused baseline. Native runtime configuration/baseline mutation and GPU uploads remain unverified.'
report['lighting_metadata_repeat_uploads']=0
report['lighting_metadata_cache_checks']='force, invalidation, plan prevalidation, failed callback retry'
report['lighting_record_capacity_failure_preserves_state']=True
report['ambient_retained_repeat_uploads']=0
report['ambient_upload_scope']='Installed generic VS20 static0/dynamic2 CTAB binding. CPU plan expands packed RGB to six float4 constants with fourth component1.0, matching byte-recovered 10012140. Native ambient upload execution and GPU behavior remain unverified.'
report['pair_scope']='Explicit static choices0 and diagnostic water-write input. Recovered PS20 inline arithmetic, not original whole-helper execution or GPU rendering.'
report['policy_scope']='Owned observed-branch domain guard, not proof of original renderer input enforcement. Raw arithmetic oracle remains unchanged.'
Path('outputs/sfm64-generic-lighting-draw-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('lighting_state_to_installed_programs=144 range_rejections=16 missing_in_range=0')
