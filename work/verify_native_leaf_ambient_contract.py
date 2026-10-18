"""Read-only byte checks for the installed engine's leaf ambient interpolator.

This records static evidence, not execution parity or native lighting-anchor proof.
"""
from pathlib import Path
import hashlib
import json
import re
import struct
import tempfile
from pe_image import Image

ROOT = Path(__file__).resolve().parent.parent
ENGINE = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
DIGEST = 'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
RANGES = [
    ('model_illumination_position_and_attachment_helper', 0x100d2780, 0x100d27f4),
    ('model_explicit_or_offset_lighting_origin_selection', 0x100d5760, 0x100d57d3),
    ('model_illumination_helper_to_lighting_caller', 0x100d871f, 0x100d877d),
    ('engine_affine_point_transform', 0x10244180, 0x10244221),
    ('world_trace_position_and_leftsolid_finish', 0x10145e20, 0x10145f33),
    ('world_trace_base_result_copy', 0x10146de0, 0x10146e63),
    ('displacement_loader_face_lump_choice', 0x1014c0f5, 0x1014c12c),
    ('displacement_loader_sequential_read_requests', 0x1014c2d6, 0x1014c34d),
    ('displacement_core_complete_constructor', 0x10142a70, 0x10142b57),
    ('displacement_core_complete_geometry_create_dispatch', 0x10142180, 0x1014220f),
    ('displacement_core_complete_raw_vertex_wrapper', 0x101428e0, 0x10142a70),
    ('displacement_core_complete_input_initializer', 0x10142210, 0x101428d4),
    ('displacement_loader_face_association_scan', 0x1014c1f9, 0x1014c244),
    ('displacement_surface_nearest_start_corner', 0x1013cfa0, 0x1013d0b1),
    ('displacement_surface_rotation', 0x1013d0c0, 0x1013d567),
    ('displacement_loader_core_input_setup', 0x1014c34d, 0x1014c3c9),
    ('displacement_loader_quad_and_orientation_setup', 0x1014c3c9, 0x1014c684),
    ('displacement_core_encoded_flags_setup', 0x10142210, 0x1014225e),
    ('leaf_ambient_interpolator', 0x100e15f0, 0x100e1901),
    ('rgbexp_to_vector', 0x102485c0, 0x10248636),
    ('lighting_caller', 0x1019323f, 0x10193258),
    ('leaf_center_half_extent_loader', 0x100e099d, 0x100e0a45),
    ('ambient_mode_dispatch', 0x10193170, 0x101932e8),
    ('ambient_mode_registration', 0x1030f710, 0x1030f72e),
    ('cache_to_ambient_caller', 0x101969a0, 0x10196a40),
    ('cache_query_input_and_key', 0x10197750, 0x10197859),
    ('cache_anchor_selection', 0x1019791b, 0x101979f3),
    ('cached_static_refresh', 0x101980e9, 0x10198119),
    ('leaf_to_cluster', 0x101465b0, 0x101465c5),
    ('point_leaf_wrapper_and_traversal', 0x101455e0, 0x10145685),
    ('cache_center_registration', 0x1030f860, 0x1030f879),
    ('cache_center_candidate_and_leaf_test', 0x101947b0, 0x10194899),
    ('cache_cell_bounds', 0x10192950, 0x10192a60),
    ('native_truncate_conversion', 0x102ded50, 0x102dedfb),
    ('complete_cache_position_policy', 0x101947b0, 0x10194b41),
    ('leaf_constraint_low_level', 0x10145690, 0x101457d8),
    ('leaf_constraint_wrapper', 0x101457e0, 0x1014580c),
    ('cache_filter_should_hit', 0x102727e0, 0x102727e5),
    ('cache_filter_trace_type', 0x10127d20, 0x10127d26),
    ('trace_client_factory', 0x1016d420, 0x1016d426),
    ('trace_client_registration', 0x1030d2f0, 0x1030d304),
    ('trace_dispatch_world_and_type_gate', 0x1016ece0, 0x1016ede6),
    ('trace_dispatch_exit', 0x1016f0be, 0x1016f0d3),
    ('world_trace_wrapper', 0x10149eb0, 0x1014a12c),
    ('general_line_brush_kernel', 0x10147690, 0x1014791f),
    ('swept_line_world_traversal', 0x10148950, 0x10148c2b),
    ('swept_leaf_brush_displacement_processing', 0x10148270, 0x10148537),
    ('brush_hit_plane_copy', 0x100e9720, 0x100e975d),
    ('brush_hit_surface_lookup', 0x10146440, 0x1014646a),
    ('optimized_brush_box_kernel', 0x10146850, 0x10146c24),
    ('runtime_brush_lump_loader', 0x1014b430, 0x1014b521),
    ('optimized_box_classification', 0x1014b5a5, 0x1014b668),
    ('optimized_box_allocation_and_dispatch', 0x1014b668, 0x1014b72a),
    ('collision_plane_loader', 0x1014b140, 0x1014b427),
    ('optimized_box_record_constructor', 0x1014a5b0, 0x1014a936),
    ('stationary_brush_primitive', 0x10146c30, 0x10146dd7),
    ('stationary_leaf_processing', 0x10147f40, 0x1014802a),
    ('stationary_world_dispatch', 0x10148030, 0x10148156),
    ('stationary_surrounding_leaf_selection', 0x10145810, 0x101459a4),
    ('displacement_leaf_mapping_builder', 0x1014da10, 0x1014db41),
    ('displacement_bounds_leaf_assignment', 0x1014d610, 0x1014d852),
    ('displacement_line_trace_wrapper', 0x1014d010, 0x1014d0f8),
    ('displacement_trace_eligibility', 0x1015b520, 0x1015b53f),
    ('displacement_collision_vtable_assignment', 0x1015bdf0, 0x1015be02),
    ('displacement_collision_initialize_and_bounds_padding', 0x1015c6b0, 0x1015c754),
    ('displacement_collision_tree_bounds', 0x10159a40, 0x10159c6e),
    ('displacement_core_odd_quad_indices', 0x1013d960, 0x1013da5a),
    ('displacement_core_even_quad_indices', 0x1013da60, 0x1013db5c),
    ('displacement_core_checkerboard_dispatch', 0x1013db60, 0x1013dc29),
    ('displacement_core_index_wrapper', 0x1013dc30, 0x1013dc43),
    ('displacement_core_triangle_record_copy', 0x10141c70, 0x10141cec),
    ('displacement_collision_pair_mapping_builder', 0x1015b8e0, 0x1015b9e3),
    ('displacement_collision_node_vector_insert', 0x1015b040, 0x1015b0c7),
    ('displacement_collision_leaf_pair_vector_insert', 0x1015b0d0, 0x1015b14f),
    ('displacement_collision_triangle_plane', 0x10159170, 0x101592d6),
    ('displacement_collision_triangle_extrema', 0x101592e0, 0x10159513),
    ('displacement_triangle_plane_normalization', 0x10244060, 0x102440cd),
    ('displacement_ray_triangle_fraction', 0x1014dd40, 0x1014dfef),
    ('displacement_ray_triangle_tolerance', 0x1014dca0, 0x1014dd3a),
    ('displacement_ray_tree_walk_and_triangle_dispatch', 0x1015a130, 0x1015a565),
    ('displacement_ray_eligibility_and_trace_wrapper', 0x1015b520, 0x1015b5a3),
    ('displacement_triangle_loader_tag_alpha_setup', 0x1015c220, 0x1015c39a),
    ('displacement_complete_core_to_runtime_mesh', 0x1015bf90, 0x1015c3ad),
    ('displacement_runtime_vertex_vector_insert', 0x101599b0, 0x10159a35),
    ('displacement_runtime_triangle_vector_insert_and_construct', 0x1015b1d0, 0x1015b378),
    ('displacement_core_triangle_index_getter', 0x10141be0, 0x10141c2d),
    ('displacement_world_post_trace_solid_policy', 0x1014cfa0, 0x1014cfe9),
    ('displacement_outer_bounds_gate', 0x1014e790, 0x1014e899),
]


def require(value, message):
    if not value:
        raise RuntimeError(message)


def main():
    image = Image(ENGINE)
    require(hashlib.sha256(image.b).hexdigest() == DIGEST, 'Installed engine changed')
    rows = []
    pending = None
    for line in (ROOT / 'work/engine-disassembly.txt').read_text().splitlines():
        match = re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)', line)
        if match:
            pending = None
            address = int(match[1], 16)
            if any(a <= address < b for _, a, b in RANGES):
                pending = dict(address=address, bytes=bytes.fromhex(match[2]).hex(),
                               instruction=match[3].strip())
                rows.append(pending)
        elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+', line):
            pending['bytes'] += bytes.fromhex(line).hex()
    checked = []
    for label, start, end in RANGES:
        selected = [r for r in rows if start <= r['address'] < end]
        cursor = start
        for row in selected:
            require(row['address'] == cursor, 'Disassembly gap: ' + label)
            raw = bytes.fromhex(row['bytes'])
            offset = image.offset(cursor)
            require(image.b[offset:offset+len(raw)] == raw, 'Stale disassembly: ' + label)
            cursor += len(raw)
        require(cursor == end, 'Incomplete range: ' + label)
        checked.append(dict(name=label, start=hex(start), end_exclusive=hex(end),
                            byte_count=end-start, instructions=selected))
    constants = []
    for address, expected in [(0x1031bdec, 1.0), (0x1031cb50, 0.5),
                              (0x1031dd40, 255.0), (0x1033fa60, 0.03125),
                              (0x103423d4, -99999.0), (0x1031be7c, 0.0),
                              (0x1033e398, struct.unpack('<f', struct.pack('<f', 2/255))[0])]:
        value = struct.unpack_from('<f', image.b, image.offset(address))[0]
        require(value == expected, 'Unexpected native constant')
        constants.append(dict(address=hex(address), value=value))
    table = []
    for exponent in range(-128, 128):
        address = 0x1040ed00 + exponent*4
        value = struct.unpack_from('<f', image.b, image.offset(address))[0]
        expected = struct.unpack('<f', struct.pack('<f', 2.0**exponent / 255.0))[0]
        require(value == expected, 'Exponent table mismatch: ' + str(exponent))
        table.append(dict(exponent=exponent, value=value))
    switch = [image.word(0x101932e8+i*4) for i in range(4)]
    require(switch == [0x10193220,0x10193232,0x1019322e,0x1019323f], 'Ambient dispatch table changed')
    require(image.string(0x1035bdec) == 'r_radiosity', 'Unexpected ambient selector name')
    require(image.string(0x1031ec94) == '4', 'Unexpected ambient selector default')
    require(image.string(0x1035bee4) == 'r_lightcachecenter', 'Unexpected cache anchor selector')
    require(image.string(0x1031da24) == '1', 'Unexpected cache center default')
    require(image.word(0x1031dee4)==0x102727e0 and image.word(0x1031dee8)==0x10127d20,'Cache trace filter table changed')
    require(image.string(0x10354658)=='EngineTraceClient004','Installed trace interface version changed')
    require(image.word(0x10407e90)==0x10407ebc and image.word(0x10407ebc)==0x103544ec,'Cache trace object/vtable changed')
    require(image.word(0x103544ec+5*4)==0x1016ece0,'Installed trace dispatch slot changed')
    require(image.word(0x1035456c+5*4)==0x1016ece0,'Server/client trace dispatch differs')
    require(image.word(0x103f1260)==0x10407ff0 and image.word(0x103544e8)==0x103f1254,'Trace client RTTI chain changed')
    require(hashlib.sha256(ENGINE.read_bytes()).hexdigest() == DIGEST, 'Engine changed during audit')
    output = ROOT / 'outputs/native-leaf-ambient-contract-20261015'
    output.mkdir(parents=True, exist_ok=True)
    receipt = Path(tempfile.mkdtemp(prefix='verification-', dir=output)) / 'results.json'
    receipt.write_text(json.dumps(dict(
        status='PASS_STATIC_NATIVE_CONTRACT', engine=str(ENGINE), sha256=DIGEST,
        ranges=checked, constants=constants, exponent_table=table,
        selector=dict(name='r_radiosity', registered_default=4,
                      jump_table=[hex(a) for a in switch],
                      live_value_verified=False),
        cache_center_selector=dict(name='r_lightcachecenter', registered_default=1,
                                   live_value_verified=False),
        trace_dispatch=dict(interface='EngineTraceClient004', object_pointer=hex(0x10407e90),
                            object=hex(0x10407ebc), vtable=hex(0x103544ec), slot=5,
                            method=hex(0x1016ece0), world_call=hex(0x10149eb0),
                            world_only_type=1, world_only_exit=hex(0x1016f0be),
                            execution_verified=False),
        limitations=['No installed code executed', 'Model lighting anchor not recovered',
                     'No reconstructed interpolator tested', 'No editor rendering changed'],
    ), indent=2) + '\n')
    print(json.dumps(dict(status='PASS_STATIC_NATIVE_CONTRACT', receipt=str(receipt),
                         checked_bytes=sum(b-a for _, a, b in RANGES),
                         exponent_entries=len(table))))


if __name__ == '__main__':
    main()
