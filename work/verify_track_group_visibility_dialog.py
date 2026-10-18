import hashlib, json, re, struct, sys
from pathlib import Path
sys.path.insert(0, 'work')
from pe_image import Image
BINARY = r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll'
image = Image(BINARY)
disasm = Path('work/ifm-disassembly.txt')
def check(start, end):
    rows, current, active = [], None, False
    for line in disasm.open(errors='replace'):
        m = re.match(r'^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)', line)
        if m:
            va = int(m.group(1), 16)
            if va < start: continue
            active = True
            if current: rows.append(current)
            if va >= end: current = None; break
            current = [va, bytes.fromhex(m.group(2))]
        elif active and current and re.match(r'^\s{10,}[0-9A-F]{2}(?:\s+[0-9A-F]{2})*\s*$', line):
            current[1] += bytes.fromhex(line.strip())
    if current: rows.append(current)
    cursor, total, bad = start, 0, []
    for va, expected in rows:
        actual = image.b[image.offset(va):image.offset(va)+len(expected)]
        if va != cursor or actual != expected: bad.append({'va':hex(va),'expected_va':hex(cursor),'bytes_match':actual==expected})
        cursor = va + len(actual); total += len(actual)
    if cursor != end: bad.append({'expected_end':hex(end),'actual_end':hex(cursor)})
    return {'start':hex(start),'end_exclusive':hex(end),'bytes':total,'all_match':not bad,'mismatches':bad}

def imported_symbols(slot_addresses):
    """Resolve selected IAT slots against the installed PE import descriptors."""
    data = image.b
    pe = image.u32(60)
    optional = pe + 24
    import_rva = image.u32(optional + 96 + 8)
    descriptor = image.offset(image.base + import_rva)
    wanted = set(slot_addresses)
    found = {}

    def c_string(offset):
        return data[offset:data.index(b'\0', offset)].decode('ascii', 'replace')

    while True:
        original_thunk, timestamp, forwarder, name_rva, first_thunk = struct.unpack_from('<IIIII', data, descriptor)
        if not any((original_thunk, timestamp, forwarder, name_rva, first_thunk)):
            break
        library = c_string(image.offset(image.base + name_rva))
        names_offset = image.offset(image.base + (original_thunk or first_thunk))
        index = 0
        while True:
            entry = image.u32(names_offset + index * 4)
            if not entry:
                break
            iat_va = image.base + first_thunk + index * 4
            if iat_va in wanted:
                symbol = f'ordinal_{entry & 0xffff}' if entry & 0x80000000 else c_string(image.offset(image.base + entry) + 2)
                found[iat_va] = {'library': library, 'symbol': symbol}
            index += 1
        descriptor += 20
    return {hex(slot): found.get(slot) for slot in sorted(wanted)}
checks = {
 'qt_metaobject_accessor': check(0x1007CB20,0x1007CB30),
 'dialog_qt_metacall_wrapper': check(0x1007CAD0,0x1007CB13),
 'dialog_local_static_metacall': check(0x1007CA10,0x1007CA61),
 'cell_changed_slot': check(0x1010CA30,0x1010CA57),
 'row_column_cell_change_handler': check(0x1010C140,0x1010C2A3),
 'special_row_last_entry_resolver': check(0x1027CB80,0x1027CBA8),
 'track_group_collection_count_accessor': check(0x1008C0E0,0x1008C0E4),
 'text_attribute_update_helper': check(0x10366100,0x103661C9),
 'boolean_attribute_update_helper': check(0x10366260,0x10366365),
 'string_attribute_undo_record_constructor': check(0x10365770,0x10365842),
 'boolean_attribute_undo_record_constructor': check(0x10365850,0x103658E9),
 'ok_slot': check(0x1010CA60,0x1010CA71),
 'done_slot': check(0x1010CA80,0x1010CAD7),
 'dialog_option_table_setup': check(0x1010BFE0,0x1010C0EE),
 'dialog_table_layout_helper': check(0x1010BE20,0x1010BFD2),
 'dialog_track_group_row_population': check(0x1010C460,0x1010C54C),
 'dialog_table_item_factory': check(0x1010C2B0,0x1010C45A),
 'named_attribute_lookup': check(0x1035DD50,0x1035DD91),
 'string_attribute_value_equality_guard': check(0x10365BA0,0x10365C00),
 'boolean_attribute_value': check(0x10058790,0x100587E3),
 'dialog_factory': check(0x1010C550,0x1010C7D0),
 'menu_callback': check(0x1010C7D0,0x1010C802),
 'modal_dialog_service': check(0x106A0960,0x106A09A0),
}
sha = hashlib.sha256(image.b).hexdigest()
assert sha == '8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4'
assert all(item['all_match'] for item in checks.values()), checks
assert image.b[image.offset(0x1010C682):image.offset(0x1010C68E)] == bytes.fromhex('E8 59 F9 FF FF 8B CE E8 D2 FD FF FF')
assert image.word(0x1010C57F) == 0x10E7D86C
assert image.word(0x1010C585) == 0x10AB70A4
assert image.word(0x10AB70AC) == 0x1007CAD0
assert image.string(0x10AA3A80) == 'CQTrackGroupVisibilityDialog'
assert image.string(0x10AA3AAB) == 'OnCellChanged(int,int)'
assert image.string(0x10AA3AC2) == 'OnOK()'
assert image.string(0x10AA3ACB) == 'done(int)'
assert image.string(image.word(image.word(0x10B38258 - 4) + 12) + 8) == '.?AV?$CUndoAttributeSetValueElement@_N@@'
assert image.string(image.word(image.word(0x10B3820C - 4) + 12) + 8) == '.?AV?$CUndoAttributeSetValueElement@VCUtlSymbolLarge@@@@'
qt_imports = imported_symbols([
 0x10A7B954, 0x10A7BBFC, 0x10A7BBB0, 0x10A7BBBC, 0x10A7BBC0,
 0x10A7BC18, 0x10A7CC00, 0x10A7CC58, 0x10A7D07C, 0x10A7D1A0,
 0x10A7D248, 0x10A7D250, 0x10A7C488, 0x10A7BC30,
 0x10A7BC10, 0x10A7C490, 0x10A7C494, 0x10A7BC0C, 0x10A7D69C,
 0x10A7D244, 0x10A7BBB4, 0x10A7C498, 0x10A7D240,
 0x10A7C6CC, 0x10A7B310,
 0x10A7BBAC, 0x10A7BBCC, 0x10A7C75C,
])
expected_imports = {
 '0x10a7b954': ('QtCore4.dll', '?connect@QObject@@SA_NPBV1@PBD01W4ConnectionType@Qt@@@Z'),
 '0x10a7bbfc': ('QtGui4.dll', '??0QPushButton@@QAE@ABVQString@@PAVQWidget@@@Z'),
 '0x10a7bbb0': ('QtGui4.dll', '?setItem@QTableWidget@@QAEXHHPAVQTableWidgetItem@@@Z'),
 '0x10a7bbbc': ('QtGui4.dll', '?setColumnCount@QTableWidget@@QAEXH@Z'),
 '0x10a7bbc0': ('QtGui4.dll', '?setRowCount@QTableWidget@@QAEXH@Z'),
 '0x10a7bc18': ('QtGui4.dll', '??0QTableWidget@@QAE@PAVQWidget@@@Z'),
 '0x10a7cc00': ('QtGui4.dll', '?setWindowTitle@QWidget@@QAEXABVQString@@@Z'),
 '0x10a7cc58': ('QtGui4.dll', '?setMinimumSize@QWidget@@QAEXHH@Z'),
 '0x10a7d07c': ('QtGui4.dll', '??0QDialogButtonBox@@QAE@PAVQWidget@@@Z'),
 '0x10a7d1a0': ('QtGui4.dll', '??0QGridLayout@@QAE@PAVQWidget@@@Z'),
 '0x10a7d248': ('QtGui4.dll', '?addButton@QDialogButtonBox@@QAEXPAVQAbstractButton@@W4ButtonRole@1@@Z'),
 '0x10a7d250': ('QtGui4.dll', '?setAutoDefault@QPushButton@@QAEX_N@Z'),
 '0x10a7c488': ('QtGui4.dll', '?isChecked@QAbstractButton@@QBE_NXZ'),
 '0x10a7bc30': ('QtGui4.dll', '?setEnabled@QWidget@@QAEX_N@Z'),
 '0x10a7bc10': ('QtGui4.dll', '?horizontalHeader@QTableView@@QBEPAVQHeaderView@@XZ'),
 '0x10a7c490': ('QtGui4.dll', '?setResizeMode@QHeaderView@@QAEXHW4ResizeMode@1@@Z'),
 '0x10a7c494': ('QtGui4.dll', '?setColumnWidth@QTableView@@QAEXHH@Z'),
 '0x10a7bc0c': ('QtGui4.dll', '?setStretchLastSection@QHeaderView@@QAEX_N@Z'),
 '0x10a7d69c': ('QtGui4.dll', '?exec@QDialog@@QAEHXZ'),
 '0x10a7d244': ('QtGui4.dll', '??0QTableWidgetItem@@QAE@H@Z'),
 '0x10a7bbb4': ('QtGui4.dll', '?setFlags@QTableWidgetItem@@QAEXV?$QFlags@W4ItemFlag@Qt@@@@@Z'),
 '0x10a7c498': ('QtGui4.dll', '?setCheckState@QTableWidgetItem@@QAEXW4CheckState@Qt@@@Z'),
 '0x10a7d240': ('QtGui4.dll', '?setSizeHint@QTableWidgetItem@@QAEXABVQSize@@@Z'),
 '0x10a7c6cc': ('QtGui4.dll', '?text@QLineEdit@@QBE?AVQString@@XZ'),
 '0x10a7b310': ('QtCore4.dll', '?toFloat@QString@@QBEMPA_N@Z'),
 '0x10a7bbac': ('QtGui4.dll', '?item@QTableWidget@@QBEPAVQTableWidgetItem@@HH@Z'),
 '0x10a7bbcc': ('QtGui4.dll', '?text@QTableWidgetItem@@QBE?AVQString@@XZ'),
 '0x10a7c75c': ('QtGui4.dll', '?checkState@QTableWidgetItem@@QBE?AW4CheckState@Qt@@XZ'),
}
assert {slot: (value['library'], value['symbol']) for slot, value in qt_imports.items()} == expected_imports
assert image.string(0x10A8E744) == 'name'
assert image.string(0x10AA7D40) == 'visible'
receipt = {
 'binary':BINARY,'sha256':sha,'class':'CQTrackGroupVisibilityDialog','qt_metaobject_va':'0x10E7D86C',
 'method_strings':{'class':'CQTrackGroupVisibilityDialog','parameters':'nRow,nColumn','slot_0':'OnCellChanged(int,int)','slot_1':'OnOK()','slot_2':'done(int)'},
 'dispatch':{'vtable':'0x10AB70A4','qt_metacall':'0x1007CAD0','local_static_metacall':'0x1007CA10','slot_0':'0x1010C140','slot_1':'0x1010C0F0','slot_2':'Qt base-class dispatch','neighboring_class_dispatch_not_used':'0x1007CB30'},
 'undo_write_path':{
  'service':'global undo-service interface at 0x10E9A51C',
  'recording_gate':'virtual slot +0xF8 queried with the changed owner handle',
  'string_attribute_record':{'type':'CUndoAttributeSetValueElement<CUtlSymbolLarge>','constructor':'0x10365770','allocation_size':36},
  'boolean_attribute_record':{'type':'CUndoAttributeSetValueElement<bool>','constructor':'0x10365850','allocation_size':32},
  'submit':'virtual slot +0x13C receives the constructed undo record before the setter commits the new attribute value',
  'scope':'statically verified setter path; live undo/redo replay and session-level behavior are not verified'
 },
 'dialog_ui':{
  'title':'Track Group Visibility',
  'table_type':'QTableWidget',
  'table_column_count':2,
  'column_headers':['', 'Track Group Name'],
  'column_0_width':20,
  'last_header_section_stretches':True,
  'initialization_order':{'signal_connection_helper':'0x1010BFE0','row_population_helper':'0x1010C460','constructor_calls_connect_helper_before_population':True,'row_insertion':'QTableWidget::setItem'},
  'row_population':{
   'row_count':'track-group count plus one special row',
   'row_0':{'resolved_through':'virtual slot +0x5C on the last object returned by 0x1027CB80','last_object_lookup':'0x1027CB80 selects the final handle from a vector at object offset +0x88 using count at +0x94, then resolves it through the undo-service object lookup','identity':'not yet tied to a specific serialized DmeTrackGroup; do not equate it with subClipTrackGroup without further evidence'},
   'rows_1_through_n':'resolved from the indexed track-group collection',
   'group_count_accessor':'0x1008C0E0 reads object field +0x68',
   'column_0':{'property':'visible','item_setter':'QTableWidget::setItem'},
   'column_1':{'property':'name','item_setter':'QTableWidget::setItem'},
   'row_index':'table row index; collection lookup uses row index minus one after the special row',
  },
  'item_creation':{
   'visible_property':'copied into a user-checkable QTableWidgetItem (flags 0x30); current boolean maps to CheckState 2 or 0',
   'name_property':'converted to display text and placed in a QTableWidgetItem with Enabled and Editable flags (flags 0x22)',
   'writeback':'the connected cell-change slot dispatches to a row/column handler that updates name or visible on the resolved DmeElement; runtime persistence and undo behavior are not established',
  },
  'buttons':'QDialogButtonBox with an OK button connected to OnOK()',
  'table_minimum_size_call_arguments':[200,300],
  'display':'the shared dialog service invokes QDialog::exec()',
  'callback_return_path':'the checked menu callback passes the dialog to the modal service, does not consume the returned dialog result, and returns immediately after the call',
 'cell_change_signal':'QTableWidget::cellChanged(int,int) connected to OnCellChanged(int,int); this dialog vtable routes Qt metacall to local static metacall 0x1007CA10, whose method id 0 calls row/column handler 0x1010C140',
 },
 'static_observations':[
  'The constructor creates a two-column QTableWidget with a blank column-0 header and Track Group Name as the column-1 header, sets column 0 to width 20, enables stretch-last-section, sets the table widget minimum size to 200x300, and connects cellChanged(int,int) to the dialog slot.',
  'The constructor tail-calls a layout helper that reads the table pointer at this+0x14; that helper contains no assignment to the QLineEdit pointer consumed by done(int) at this+0x1C. The derived constructor body likewise does not assign this+0x1C. This does not rule out state initialized by the QDialog base constructor or external runtime code.',
  'The derived constructor calls helper 0x1010BFE0 (which connects cellChanged) immediately before row-population helper 0x1010C460; row insertion uses QTableWidget::setItem. Qt 4 source shows setItem updates emit dataChanged, which QTableWidget forwards to itemChanged/cellChanged. Thus population may synchronously enter the connected slot; this ordering is statically established, while live SFM signal dispatch during construction is not runtime-observed.',
  'The row-population helper sets row count to track-group count plus one, resolves table row 0 through a separate virtual accessor, and resolves later rows from the indexed collection; it inserts name in column 1 and visible in column 0.',
  'Follow-up disassembly confirms row 0 calls virtual slot +0x5C on the final object returned by 0x1027CB80. That helper selects the last handle from an internal vector at +0x88 (count at +0x94) and resolves the handle through the global undo-service object lookup. The count helper 0x1008C0E0 reads field +0x68. These structures explain the extra row and indexed rows, but the last object has not been mapped to a specific serialized DmeTrackGroup; the benchmark element named subClipTrackGroup is not assumed to be row 0.',
  'The cell-item factory resolves the named DmeElement attribute; `visible` is read as a boolean and copied to a checkable QTableWidgetItem, while `name` becomes display text in an enabled/editable QTableWidgetItem.',
  'The dialog vtable at 0x10AB70A4 identifies 0x1007CAD0 as its qt_metacall wrapper. That wrapper delegates through Qt and then calls this class local static metacall at 0x1007CA10 for local method ids 0 through 2. The static metacall maps method id 0 to 0x1010C140 with the two signal arguments; the nearby dispatcher at 0x1007CB30 belongs to another class and is not this dialog’s route.',
  'The connected row/column handler at 0x1010C140 resolves row 0 through the special-element accessor and later rows through collection index row-1, fetches QTableWidget::item(row,column), and branches by column: column 1 reads item text and sends it to the element name attribute updater; column 0 converts CheckState==2 to bool and sends it to the element visible attribute updater. The helper bodies directly write attribute values and notify the data-model layer.',
  'A QDialogButtonBox is created with the OK standard button and its clicked() signal is connected to OnOK().',
  'The shared dialog service calls QDialog::exec() on the constructed dialog, confirming modal display invocation in the static call path.',
  'The checked menu callback does not read the modal result after the service call; it returns immediately. No post-dialog result-based commit is visible in this callback.',
  'The neighboring-class method at 0x1010CA30 calls QAbstractButton::isChecked() twice and controls widget state; the current dialog vtable/meta-call evidence shows it is not this table signal route.',
  'OnOK calls QAbstractButton::isChecked() on the widget at +0x20 and stores the returned byte at dialog offset +0x3C. No track-group data write is visible in this slot body.',
  'done(int) reads text from the QLineEdit stored at dialog offset +0x1C, converts it with QString::toFloat, and conditionally stores the float at +0x34. This is a numeric-control path; its relationship to the visibility table is not established.',
  'Static code establishes the edit write path from the connected cellChanged signal through this class’s Qt meta-call to attribute writes. Runtime dispatch, undo integration, save/reopen persistence, and rendered visibility changes remain unverified.',
 ],
 'qt_imports':qt_imports,
 'initialization_noop_analysis':{'connect_precedes_population':True,'qt_table_item_insertion_emits_cell_changed':'Qt 4 source behavior; exact live SFM dispatch during construction not observed','boolean_same_value_skips_undo_and_mutation':True,'string_same_value_guard':'symbol pointer comparison; display-text conversion equivalence not fully traced'},
 'checks':checks,'all_match':True
}
Path('outputs/timeline-track-group-visibility-dialog-verification.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps({'sha256':sha,'checks':{k:{'bytes':v['bytes'],'all_match':v['all_match']} for k,v in checks.items()},'all_match':True},indent=2))
