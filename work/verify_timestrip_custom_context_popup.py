import hashlib
import json
import re
import struct
from pathlib import Path

from pe_image import Image
from resolve_pe_iat_slot import resolve


BINARY = Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
LISTING = Path("work/ifm-disassembly.txt")
OUT = Path("outputs/timestrip-custom-context-popup-linkage-verification.json")
IMAGE = Image(BINARY)
ifm_sha256 = hashlib.sha256(IMAGE.b).hexdigest()

rows = {}
current = None
for line in LISTING.open(errors="replace"):
    match = re.match(r"^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)", line)
    if match:
        current = {"va": int(match.group(1), 16), "bytes": bytearray.fromhex(match.group(2))}
        rows[current["va"]] = current
    elif current and re.match(r"^\s{10,}[0-9A-F]{2}(?:\s+[0-9A-F]{2})*\s*$", line):
        current["bytes"].extend(bytes.fromhex(line.strip()))


def verify_span(start, end):
    cursor = start
    expected = bytearray()
    instructions = []
    for va in sorted(address for address in rows if start <= address < end):
        row = rows[va]
        if va != cursor:
            raise AssertionError(f"gap/overlap in listing: expected {cursor:#x}, got {va:#x}")
        expected.extend(row["bytes"])
        instructions.append({"va": hex(va), "bytes": row["bytes"].hex()})
        cursor = va + len(row["bytes"])
    if cursor != end:
        raise AssertionError(f"incomplete span {start:#x}-{end:#x}: stopped at {cursor:#x}")
    live_offset = IMAGE.offset(start)
    live = IMAGE.b[live_offset:live_offset + len(expected)]
    if live != expected:
        raise AssertionError(f"installed bytes mismatch at {start:#x}")
    return {
        "start": hex(start),
        "end_exclusive": hex(end),
        "instruction_rows": len(instructions),
        "bytes": len(expected),
        "all_bytes_match_installed": True,
        "instructions": instructions,
    }


spans = [
    verify_span(0x1019B10B, 0x1019B18A),  # widget vtable setup and Qt signal/slot connect
    verify_span(0x1007C7A0, 0x1007C7E6),  # CQTimestripWidget meta-call dispatcher
    verify_span(0x1007C660, 0x1007C6AE),  # local slot 0 -> menu builder
    verify_span(0x100D5DEE, 0x100D5EB1),  # Graph Editor upper/lower time strip construction
    verify_span(0x1019B0C0, 0x1019B2AC),  # complete CQTimestripWidget base initializer
    verify_span(0x1019BD80, 0x1019BDB7),  # CQUpperTimestripWidget derived constructor
    verify_span(0x1019C020, 0x1019C057),  # CQLowerTimestripWidget derived constructor
]

vtable_va = 0x10AC75E4
vtable_bytes = IMAGE.b[IMAGE.offset(vtable_va):IMAGE.offset(vtable_va) + 12]
vtable_targets = list(struct.unpack("<III", vtable_bytes))
if vtable_targets != [0x1007C9C0, 0x1007C6B0, 0x1007C7A0]:
    raise AssertionError(f"unexpected CQTimestripWidget vtable entries: {vtable_targets!r}")

graph_editor_strip_labels = {
    "upper": IMAGE.string(0x10AAE874),
    "lower": IMAGE.string(0x10AAE854),
}
if graph_editor_strip_labels != {
    "upper": "Graph Editor Upper Time Strip",
    "lower": "Graph Editor Lower Time Strip",
}:
    raise AssertionError(f"unexpected Graph Editor child labels: {graph_editor_strip_labels!r}")

derived_strips = []
for class_name, vtable in (
    ("CQUpperTimestripWidget", 0x10AC76FC),
    ("CQLowerTimestripWidget", 0x10AC792C),
):
    locator = IMAGE.word(vtable - 4)
    descriptor = IMAGE.word(locator + 12)
    rtti_name = IMAGE.string(descriptor + 8)
    if rtti_name != f".?AV{class_name}@@":
        raise AssertionError(f"unexpected RTTI for {vtable:#x}: {rtti_name!r}")
    derived_strips.append({
        "class_name": class_name,
        "vtable_va": hex(vtable),
        "vtable_locator_va": hex(locator),
        "type_descriptor_va": hex(descriptor),
        "rtti_name": rtti_name,
        "base_widget_constructor": "0x1019B0C0",
    })

# Read the current PE's Qt meta-object table to verify that local id 0 is the
# custom-context slot; this is data evidence from the installed image itself.
metaobject_va = 0x10E7D844
method_strings_va = 0x10AA37A8
meta_data_va = IMAGE.word(metaobject_va + 8)
meta_header = [IMAGE.word(meta_data_va + 4 * index) for index in range(14)]
method_row_va = meta_data_va + 4 * meta_header[5]
method_row = [IMAGE.word(method_row_va + 4 * index) for index in range(5)]
method_signature_va = method_strings_va + method_row[0]
method_signature = IMAGE.string(method_signature_va)
if method_signature != "OnCustomContextMenuRequested(QPoint)":
    raise AssertionError(f"unexpected local method 0: {method_signature!r}")

connect_strings = {
    "slot_signature": IMAGE.string(0x10AAC3CC),
    "signal_signature": IMAGE.string(0x10AAC39C),
}
expected_connect_strings = {
    "slot_signature": "1OnCustomContextMenuRequested( const QPoint & )",
    "signal_signature": "2customContextMenuRequested( const QPoint & )",
}
if connect_strings != expected_connect_strings:
    raise AssertionError(f"unexpected Qt connect signatures: {connect_strings!r}")

iat_imports = [
    resolve(BINARY, address)
    for address in (0x10A7B954, 0x10A7CDB0, 0x10A7BE78, 0x10A7C444)
]

receipt = {
    "binary": str(BINARY),
    "binary_sha256": ifm_sha256,
    "vtable": {
        "va": hex(vtable_va),
        "bytes": vtable_bytes.hex(),
        "targets": [hex(value) for value in vtable_targets],
        "interpretation": "The class dispatch table selects metacast 0x1007C6B0 and metacall 0x1007C7A0 for CQTimestripWidget metaobject 0x10E7D844.",
    },
    "metaobject_slot": {
        "metaobject_va": hex(metaobject_va),
        "method_data_va": hex(meta_data_va),
        "method_row_va": hex(method_row_va),
        "local_id": 0,
        "signature_va": hex(method_signature_va),
        "signature": method_signature,
        "row_words": [hex(value) for value in method_row],
    },
    "widget_signal_setup": {
        "constructor_code_span": spans[0],
        "complete_base_initializer_span": spans[4],
        "same_object_as_sender_and_receiver": True,
        "vtable_written_to_object": hex(vtable_va),
        "context_menu_policy_value": 3,
        "connect_strings": connect_strings,
        "qt_imports": iat_imports,
        "interpretation": "The object initialized with the CQTimestripWidget vtable sets its context-menu policy to value 3 and calls QObject::connect with customContextMenuRequested as sender signal and OnCustomContextMenuRequested(QPoint) as receiver slot, using the same object for both.",
    },
    "meta_call_to_popup": {
        "meta_call_span": spans[1],
        "meta_call_forwards_local_id_0_to": "0x1007C660",
        "dispatch_span": spans[2],
        "dispatch_behavior": "For Qt call type 0 and local method id 0, the dispatcher obtains the QPoint argument from the Qt argument array and calls 0x1019C6E0 on the CQTimestripWidget instance.",
        "popup_builder_receipt": "outputs/timestrip-menu-provider-callchain-verification.json",
        "popup_builder_byte_verified": True,
        "popup_calls_bookmark_curve_key_provider": "0x100B25E0 at 0x1019C8D5",
        "popup_uses_QMenu_exec": True,
    },
    "graph_editor_instances": {
        "setup_span": spans[3],
        "named_child_widgets": graph_editor_strip_labels,
        "upper_constructor_callsite": "0x100D5E09",
        "upper_constructor": "0x1019BD80",
        "upper_constructor_span": spans[5],
        "lower_constructor_callsite": "0x100D5E9E",
        "lower_constructor": "0x1019C020",
        "lower_constructor_span": spans[6],
        "derived_rtti_and_vtables": derived_strips,
        "interpretation": "The Graph Editor UI setup block has explicit Graph Editor Upper/Lower Time Strip object labels and constructs the CQUpperTimestripWidget and CQLowerTimestripWidget classes. Both derived constructors call the verified CQTimestripWidget initializer, so both Graph Editor strips inherit its custom-context signal connection and popup dispatch.",
    },
    "conclusion": "The installed Graph Editor setup constructs upper and lower TimeStrip widgets from CQTimestripWidget, whose custom-context signal connects to OnCustomContextMenuRequested and dispatches to the verified Timestrip popup builder and Bookmark/Curve/key action provider. This proves the TimeStrip popup path is used in Graph Editor, without claiming that the separate Graph Editor viewport/tree context routes are covered.",
    "limits": [
        "Graph Editor upper and lower TimeStrip instances are identified from direct setup labels and constructor calls; the same base widget is also used by Motion Editor timestrip subclasses.",
        "The separate CQGraphEditorView selection/hit-test handler and Graph Editor panel OpenContextMenu service relay remain unlinked to this widget popup path.",
        "No live SFM menu was exercised; item presentation, ordering, enablement and callback effects remain static findings.",
        "No x64 replacement UI or application behavior is implemented by this receipt.",
    ],
}
OUT.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
print(json.dumps({
    "binary_sha256": ifm_sha256,
    "spans": [{key: span[key] for key in ("start", "end_exclusive", "instruction_rows", "bytes", "all_bytes_match_installed")} for span in spans],
    "vtable_targets": receipt["vtable"]["targets"],
    "method_signature": method_signature,
    "graph_editor_strip_labels": graph_editor_strip_labels,
    "derived_strips": derived_strips,
    "connect_strings": connect_strings,
    "iat_imports": iat_imports,
    "receipt": str(OUT),
}, indent=2))
