import hashlib, json, re, sys
from pathlib import Path
sys.path.insert(0, "work")
from pe_image import Image

binary = Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
disassembly = Path("work/ifm-disassembly.txt")
image = Image(binary)
ranges = {
    "group_selected_dags_callback": (0x1008ED10, 0x1008EE33),
    "selection_membership_lookup": (0x104C27F0, 0x104C2815),
    "selection_count_field_reader": (0x104C2B00, 0x104C2B07),
    "selection_item_resolver": (0x104C2E70, 0x104C2EC6),
    "group_child_add_unique": (0x104C5EF0, 0x104C5F2A),
    "group_element_factory_wrapper": (0x1008CEC0, 0x1008CF1E),
    "selection_type_flag_predicate": (0x1008E3D0, 0x1008E420),
    "selection_compatibility_validator": (0x1008E420, 0x1008E499),
    "selection_descriptor_resolver": (0x1008E200, 0x1008E25E),
    "animation_set_editor_selection_state_update": (0x1008D600, 0x1008D7E0),
    "selected_item_mutation_helper": (0x1008C2F0, 0x1008C334),
    "selected_reference_array_setter": (0x1036D6C0, 0x1036D777),
    "indexed_reference_array_removal": (0x103848D0, 0x103849E9),
    "context_array_remove_wrapper": (0x103885E0, 0x1038860B),
    "context_array_remove_dispatch": (0x104C2450, 0x104C245F),
    "group_context_children_resolver": (0x104C2B50, 0x104C2B9A),
    "named_attribute_reference_resolver": (0x10223170, 0x102231F9),
    "element_reference_filter": (0x10222DE0, 0x10222E76),
}
rows = {start: [] for start, _ in ranges.values()}
current = None
with disassembly.open(errors="replace") as source:
    for line in source:
        match = re.match(r"^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)", line)
        if match:
            address = int(match.group(1), 16)
            if current is not None and address in rows:
                current = None
            for start, end in ranges.values():
                if address == start:
                    current = (start, end, [])
                    break
            if current is not None:
                start, end, chunks = current
                if address >= end:
                    current = None
                else:
                    chunks.append((address, bytes.fromhex(match.group(2))))
                    rows[start] = chunks
        elif current is not None and re.match(r"^\s{10,}[0-9A-F]{2}(?:\s+[0-9A-F]{2})*\s*$", line):
            current[2][-1] = (current[2][-1][0], current[2][-1][1] + bytes.fromhex(line.strip()))

verified = {}
for name, (start, end) in ranges.items():
    cursor = start
    code = bytearray()
    for address, chunk in rows[start]:
        if address != cursor:
            break
        try:
            raw = image.b[image.offset(address):image.offset(address) + len(chunk)]
        except (ValueError, IndexError):
            break
        if raw != chunk:
            break
        code.extend(raw)
        cursor += len(chunk)
    verified[name] = bytes(code) if cursor == end else None

def has(name, pattern):
    code = verified[name]
    return code is not None and bytes.fromhex(pattern) in code

callback = verified["group_selected_dags_callback"]
facts = {
    "callback_checks_target_membership_before_creating_group": has("group_selected_dags_callback", "8BCBE8B13A430085C00F88E4000000"),
    "callback_uses_group1_factory_label": image.string(0x10AA7894) == "group1" and has("group_selected_dags_callback", "689478AA10"),
    "callback_reads_selected_count_and_iterates_items": has("group_selected_dags_callback", "8BCBE8513D430033F68945F885C07E63") and has("group_selected_dags_callback", "8BCBE8A8404300"),
    "callback_only_adds_non_anchor_items_when_type_predicate_returns_2": has("group_selected_dags_callback", "E8D6F5FFFF83F8027515"),
    "callback_restores_or_finalizes_shared_operation_context": has("group_selected_dags_callback", "8B0D1CA5E9108B118B820C010000FFD0"),
    "callback_begins_labeled_group_operation_scope": has("group_selected_dags_callback", "8B0D1CA5E9108B018B90080100006A0068EC6AAA1068EC6AAA10FFD2"),
    "callback_closes_scope_when_group_factory_fails": has("group_selected_dags_callback", "8B0D1CA5E9108B018B900C010000FFD2"),
    "callback_closes_scope_after_reference_edits": has("group_selected_dags_callback", "8B0D1CA5E9108B118B820C010000FFD0"),
    "selection_membership_scans_context_array_at_plus_0x84": has("selection_membership_lookup", "8B4D08E83230E7FF8945088D4508508D8E84000000"),
    "selection_count_reads_plus_0x90": has("selection_count_field_reader", "8B8190000000C3"),
    "selection_item_resolver_bounds_checks_against_plus_0x90": has("selection_item_resolver", "3B81900000007D3C"),
    "selection_item_resolver_indexes_plus_0x84": has("selection_item_resolver", "8B89840000008B0481"),
    "group_add_checks_for_duplicate_then_appends_to_plus_0x84": has("group_child_add_unique", "E8F6E3FFFF83F8FF7512578D8E84000000"),
    "factory_passes_requested_type_string_to_engine_creation_interface": has("group_element_factory_wrapper", "8B45108B4D0C8B55085650515251"),
    "type_predicate_has_exact_mask_match_result_2": has("selection_type_flag_predicate", "8B400823C17509B801000000") and has("selection_type_flag_predicate", "3BC10F95C28D4202"),
    "compatibility_validator_checks_every_selected_item_against_descriptor": has("selection_compatibility_validator", "8B0B8B70188B14B989550885F6743F") and has("selection_compatibility_validator", "3B45087410"),
    "compatibility_validator_rejects_unresolved_items": has("selection_compatibility_validator", "85F6743F"),
    "descriptor_resolver_uses_datamodel_type_query": has("selection_descriptor_resolver", "8B0D1CA5E9108B018B560C8B404852FFD0"),
    "selection_state_update_uses_controller_selected_item_count": has("animation_set_editor_selection_state_update", "8B9F9C00000085DB7E3C"),
    "selection_state_update_iterates_controller_selected_items": has("animation_set_editor_selection_state_update", "8B87900000008B0CB0"),
    "selection_state_update_logs_collect_selected_items_phase": has("animation_set_editor_selection_state_update", "682076AA10"),
    "selection_state_update_logs_selection_from_qt_phase": has("animation_set_editor_selection_state_update", "688075AA10"),
    "selection_state_update_dispatches_each_selected_item": has("animation_set_editor_selection_state_update", "8B87900000008B0CB08B118B02FFD0"),
    "callback_passes_created_group_and_context_array_to_shared_mutation_helper": has("group_selected_dags_callback", "8BCBE8A23643005056E8BB0E2D00"),
    "anchor_slot_is_replaced_by_group_through_indexed_reference_setter": has("group_selected_dags_callback", "8B45FC50568BCBE8613643008BC8E80AD5FFFF"),
    "indexed_reference_setter_writes_resolved_handle_at_array_index": has("selected_reference_array_setter", "8904B2"),
    "indexed_reference_setter_bounds_checks_slot_before_mutation": has("selected_reference_array_setter", "85F60F889F0000008B47083B700C0F8D93000000"),
    "indexed_reference_setter_skips_identical_reference": has("selected_reference_array_setter", "3B0B0F8481000000"),
    "indexed_reference_setter_runs_pre_and_post_change_hooks": has("selected_reference_array_setter", "E84967FFFF") and has("selected_reference_array_setter", "E83178FFFF") and has("selected_reference_array_setter", "E8AC77FFFF"),
    "non_anchor_context_entry_is_erased_by_index": has("group_selected_dags_callback", "568BCBE8493643008B4DFC57E8E0704300"),
    "remove_wrapper_dispatches_to_context_vector_erase": has("context_array_remove_dispatch", "81C1840000005DE98161ECFF"),
    "vector_erase_reduces_count_and_shifts_trailing_entries": has("indexed_reference_array_removal", "FF4B0C") and has("indexed_reference_array_removal", "E866065F00"),
    "group_context_resolver_interns_children_attribute": image.string(0x10A8A7D4) == "children" and has("group_context_children_resolver", "68D4A7A81068C4EAF510"),
    "group_context_resolver_passes_supplied_element_and_attribute_to_resolver": has("group_context_children_resolver", "8B0DC4EAF5108BC45689") and has("group_context_children_resolver", "E8DB05D6FF"),
    "named_attribute_resolver_queries_datamodel_by_attribute_symbol": has("named_attribute_reference_resolver", "8B96CC01000050FFD2"),
    "named_attribute_resolver_uses_element_relationship_field": has("named_attribute_reference_resolver", "8B7F38"),
    "named_attribute_resolver_walks_references_until_filter_matches": has("named_attribute_reference_resolver", "E80EFCFFFF85C0751C") and has("named_attribute_reference_resolver", "8B82D001000056FFD0"),
    "element_reference_filter_resolves_handle_and_checks_descriptor_flags": has("element_reference_filter", "8B13") and has("element_reference_filter", "8B8064010000") and has("element_reference_filter", "66C1E90BF6C101") and has("element_reference_filter", "66C1EA0CF6C201"),
}
mask_values = [image.word(0x10AA73B8 + 4 * i) for i in range(13)]
mask_table_matches = mask_values == [0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xE, 0x2, 0x4, 0x8, 0x70, 0x10, 0x20, 0x40]
receipt = {
    "binary": str(binary),
    "sha256": hashlib.sha256(image.b).hexdigest(),
    "method": "Group Selected Dags callback, selection, and children-context resolver byte verification",
    "ranges": {name: {"start": hex(start), "end_exclusive": hex(end), "bytes": len(verified[name]) if verified[name] else 0, "disassembly_matches_installed_bytes": verified[name] is not None} for name, (start, end) in ranges.items()},
    "verified_findings": facts,
    "selection_type_mask_table_first_13_dwords": [hex(v) for v in mask_values],
    "selection_type_mask_table_matches_expected_values": mask_table_matches,
    "inferred_control_flow": [
        "The callback rejects a null input and returns unless the input DAG occurs in the active selection collection.",
        "It creates an element using the type string group1, passing the active data-model scope to the factory wrapper.",
        "It walks the context reference array by index. The anchor slot is written to the new group, the anchor is appended to the group, and non-anchor entries are erased from the context array and added to the group only when the type/flag predicate returns exactly 2.",
        "The unique-add helper searches the destination collection at +0x84 before appending an item. Indexed removal reduces the context array count and shifts later entries down.",
        "The callback's context producer resolves the literal children attribute through the datamodel and supplies the target element to the reference resolver. The resolver reads the supplied element's +0x38 relationship field, obtains the named attribute, and filters references through descriptor/class flags. This supports a children-reference interpretation, while the exact UI owner and type meanings remain unresolved.",
    ],
    "limits": [
        "The complete edit transaction payload and reverse operation are not recovered by these spans.",
        "The context object owning the +0x84/+0x90 array and its exact relation to SFM parent DAGs are not identified by these spans.",
        "The type/flag return value 2 means the candidate flags exactly equal the mask selected by the descriptor type index; the user-facing type semantics for each table index remain unidentified.",
        "No live SFM invocation, saved-session diff, or undo/redo experiment was performed.",
        "No replacement UI action is added until the context-array owner, predicate semantics, and undo behavior are recovered.",
    ],
}
receipt["all_ranges_match"] = all(v is not None for v in verified.values())
receipt["all_facts_match"] = all(facts.values()) and mask_table_matches
Path("outputs/group-selected-dags-static-verification.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
Path("work/ifm-group-selected-dags.txt").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
print(json.dumps({"sha256": receipt["sha256"], "ranges": {k: v["bytes"] for k, v in receipt["ranges"].items()}, "all_ranges_match": receipt["all_ranges_match"], "facts": facts}, indent=2))
if not receipt["all_ranges_match"] or not receipt["all_facts_match"]:
    sys.exit(1)
