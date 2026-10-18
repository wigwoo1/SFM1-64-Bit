@echo off
setlocal
cd /d "%~dp0"
set "SFM_VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%SFM_VCVARS%" (
  echo MSVC x64 build tools not found at configured path.
  exit /b 1
)
if not exist "build\session-core" mkdir "build\session-core"
if not exist "build\temp" mkdir "build\temp"
set "TEMP=%CD%\build\temp"
set "TMP=%CD%\build\temp"
call "%SFM_VCVARS%" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\sfm64_session_core.cpp /Fo:build\session-core\sfm64_session_core.obj /Fe:build\session-core\sfm64-session-core.exe
if errorlevel 1 exit /b 1
for %%T in (test_hierarchy_scale test_owned_scalar_float_write test_owned_expression_boundaries test_owned_attachment_listeners test_registered_operator_build test_undo_journal test_document_scalar_edit test_document_float_keys test_owned_session_expression_execute test_cached_reference_comparison test_parallel_expression_samples test_parallel_private_expression_chains test_session_expression_runtime test_parallel_prepared_expressions test_reference_owned_scene_cleanup_x64 test_owned_scalar_edit_x64 test_owned_scalar_replay_x64 test_owned_undo_retention_x64 test_reference_owned_dmx_projection_x64 test_reference_owned_dmx_external_x64 test_reference_dmx_export_x64 test_owned_session_ik_pass_x64 test_owned_typed_write_x64 test_owned_selected_ik_x64 test_session_ik_selected_controls test_owned_vector_undo_x64 test_owned_quaternion_undo_x64 test_owned_ik_undo_pass_x64 test_float_undo_capture_x64 test_session_play_expression test_cli_expression_graph_x64 test_discovered_expression_chains_x64 test_expression_chain_plan_x64 test_expression_chain_pass_x64 test_hierarchy_typed_play_x64 test_combined_typed_playback_x64 test_combined_scalar_playback_x64 test_hierarchy_color_copy_x64 test_combined_color_playback_x64 test_session_pack_color_x64 test_combined_pack_color_playback_x64 test_native_flex_resize_x64 test_native_flex_index_x64 test_mdl_flex_metadata_x64 test_native_global_flex_registry_x64 test_native_model_flex_lookup_x64 test_native_flex_binding_lifecycle_x64 test_native_model_cache_refresh_x64 test_disk_model_metadata_cache_x64 audit_session_model_flex_binding_x64 test_native_lazy_flex_binding_x64 test_native_resolve_queue_dirty_x64 test_native_resolve_queue_dispatch_x64 test_native_reentrant_queue_x64 test_owned_resolve_queue_x64 test_owned_class_resolve_registry_x64) do (
  cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\%%T.cpp /Fo:build\session-core\%%T.obj /Fe:build\session-core\%%T.exe
  if errorlevel 1 exit /b 1
)
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_native_float_array_write_x64.cpp /Fo:build\session-core\test_native_float_array_write_x64.obj /Fe:build\session-core\test_native_float_array_write_x64.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_native_float_array_owner_gates_x64.cpp /Fo:build\session-core\test_native_float_array_owner_gates_x64.obj /Fe:build\session-core\test_native_float_array_owner_gates_x64.exe
if errorlevel 1 exit /b 1
for %%T in (test_owned_float_array_write_x64 test_owned_session_flex_weights_x64) do (
  cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\%%T.cpp /Fo:build\session-core\%%T.obj /Fe:build\session-core\%%T.exe
  if errorlevel 1 exit /b 1
)
for %%T in (test_native_flex_execute_x64 test_owned_global_flex_execute_x64) do (
  cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\%%T.cpp /Fo:build\session-core\%%T.obj /Fe:build\session-core\%%T.exe
  if errorlevel 1 exit /b 1
)
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_owned_global_flex_binding_plan_x64.cpp /Fo:build\session-core\test_owned_global_flex_binding_plan_x64.obj /Fe:build\session-core\test_owned_global_flex_binding_plan_x64.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_fresh_flex_pipeline_graph_x64.cpp /Fo:build\session-core\test_fresh_flex_pipeline_graph_x64.obj /Fe:build\session-core\test_fresh_flex_pipeline_graph_x64.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_flex_weight_writer_groups_x64.cpp /Fo:build\session-core\test_flex_weight_writer_groups_x64.obj /Fe:build\session-core\test_flex_weight_writer_groups_x64.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_session_flex_writer_conflict_x64.cpp /Fo:build\session-core\test_session_flex_writer_conflict_x64.obj /Fe:build\session-core\test_session_flex_writer_conflict_x64.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_flex_binding_index_x64.cpp /Fo:build\session-core\test_flex_binding_index_x64.obj /Fe:build\session-core\test_flex_binding_index_x64.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_mdl_flex_metadata_stream_x64.cpp /Fo:build\session-core\test_mdl_flex_metadata_stream_x64.obj /Fe:build\session-core\test_mdl_flex_metadata_stream_x64.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_prepared_timeline_pass_x64.cpp /Fo:build\session-core\test_prepared_timeline_pass_x64.obj /Fe:build\session-core\test_prepared_timeline_pass_x64.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_borrowed_prepared_timeline_x64.cpp /Fo:build\session-core\test_borrowed_prepared_timeline_x64.obj /Fe:build\session-core\test_borrowed_prepared_timeline_x64.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_session_runtime.cpp /Fo:build\session-core\test_session_runtime.obj /Fe:build\session-core\test_session_runtime.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_prepared_timeline_flex_x64.cpp /Fo:build\session-core\test_prepared_timeline_flex_x64.obj /Fe:build\session-core\test_prepared_timeline_flex_x64.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_prepared_binding_revision_x64.cpp /Fo:build\session-core\test_prepared_binding_revision_x64.obj /Fe:build\session-core\test_prepared_binding_revision_x64.exe
if errorlevel 1 exit /b 1
for %%T in (test_vector_key_sampler test_runtime_typed_vector_play test_scalar_sample_commit_x64 test_session_empty_float_play test_parallel_scalar_sampling_x64 test_parallel_prepared_timeline_flex_x64 test_switching_prepared_timeline_flex_x64 test_vector_sample_commit_x64 test_parallel_vector_sampling_x64 test_vector_prepared_timeline_flex_x64 test_switching_vector_timeline_flex_x64) do (
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\%%T.cpp /Fo:build\session-core\%%T.obj /Fe:build\session-core\%%T.exe
if errorlevel 1 exit /b 1
)
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_vvd_geometry_x64.cpp /Fo:build\session-core\test_vvd_geometry_x64.obj /Fe:build\session-core\test_vvd_geometry_x64.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_model_skinning_geometry_x64.cpp /Fo:build\session-core\test_model_skinning_geometry_x64.obj /Fe:build\session-core\test_model_skinning_geometry_x64.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_geometry_directions_x64.cpp /Fo:build\session-core\test_geometry_directions_x64.obj /Fe:build\session-core\test_geometry_directions_x64.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_geometry_pose_palette_x64.cpp /Fo:build\session-core\test_geometry_pose_palette_x64.obj /Fe:build\session-core\test_geometry_pose_palette_x64.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_parallel_geometry_deformation_x64.cpp /Fo:build\session-core\test_parallel_geometry_deformation_x64.obj /Fe:build\session-core\test_parallel_geometry_deformation_x64.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_sfm_vtx_geometry_x64.cpp /Fo:build\session-core\test_sfm_vtx_geometry_x64.obj /Fe:build\session-core\test_sfm_vtx_geometry_x64.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_sfm_model_geometry_x64.cpp /Fo:build\session-core\test_sfm_model_geometry_x64.obj /Fe:build\session-core\test_sfm_model_geometry_x64.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_model_material_metadata_x64.cpp /Fo:build\session-core\test_model_material_metadata_x64.obj /Fe:build\session-core\test_model_material_metadata_x64.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_vmt_keyvalues_x64.cpp /Fo:build\session-core\test_vmt_keyvalues_x64.obj /Fe:build\session-core\test_vmt_keyvalues_x64.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_vtf_texture_layout_x64.cpp /Fo:build\session-core\test_vtf_texture_layout_x64.obj /Fe:build\session-core\test_vtf_texture_layout_x64.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_vtf_block_decode_x64.cpp /Fo:build\session-core\test_vtf_block_decode_x64.obj /Fe:build\session-core\test_vtf_block_decode_x64.exe
if errorlevel 1 exit /b 1
exit /b 0
