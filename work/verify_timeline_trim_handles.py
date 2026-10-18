import hashlib, json, re, struct, sys
from pathlib import Path
sys.path.insert(0, "work")
from pe_image import Image
binary = r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll"
im = Image(binary)
digest = hashlib.sha256(im.b).hexdigest()
for receipt_path in ("outputs/timeline-trim-track-action-recovery.json", "outputs/timeline-trim-trackgroup-action-recovery.json"):
    prior = json.loads(Path(receipt_path).read_text(encoding="utf-8-sig"))
    assert prior["binary_sha256"] == digest
def verify(start, end):
    rows=[]; current=None; active=False
    for line in Path("work/ifm-disassembly.txt").open(errors="replace"):
        m=re.match(r"^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)",line)
        if m:
            va=int(m.group(1),16)
            if va < start: continue
            active=True
            if current: rows.append(current)
            if va >= end: current=None; break
            current={"va":va,"bytes":bytes.fromhex(m.group(2))}
        elif active and current and re.match(r"^\s{10,}[0-9A-F]{2}(?:\s+[0-9A-F]{2})*\s*$",line):
            current["bytes"] += bytes.fromhex(line.strip())
    if current: rows.append(current)
    cursor=start; n=0; errors=[]
    for row in rows:
        off=im.offset(row["va"]); raw=row["bytes"]
        if row["va"] != cursor or im.b[off:off+len(raw)] != raw: errors.append(hex(row["va"]))
        cursor=row["va"]+len(raw); n+=len(raw)
    if cursor != end: errors.append("end="+hex(cursor))
    return {"start":hex(start),"end_exclusive":hex(end),"instruction_rows":len(rows),"bytes":n,"all_match":not errors,"mismatches":errors}
ranges={"track_trim_callback":(0x1017CC80,0x1017CDCE),"trackgroup_trim_callback":(0x1017CB30,0x1017CC7E),"per_track_trim_apply":(0x10283730,0x10283B90),"trackgroup_trim_dispatch":(0x10283B90,0x10283C25),"secondary_trim_dispatch":(0x10283C30,0x10283D20),"trim_dialog_initializer":(0x1010D040,0x1010D16B),"trim_dialog_value_setter":(0x1010C950,0x1010CA10),"trim_dialog_value_getter":(0x1010CA10,0x1010CA29),"trim_child_update_adapter":(0x105171B0,0x1051720D)}
spans={name:verify(*bounds) for name,bounds in ranges.items()}
assert all(x["all_match"] for x in spans.values()), spans
address=0x10A8A854; raw=im.b[im.offset(address):im.offset(address)+4]; value=struct.unpack("<f",raw)[0]; assert value==5.0
result={"binary":binary,"sha256":digest,"helper_spans":spans,"total_verified_bytes":sum(x["bytes"] for x in spans.values()),"default_float":{"address":hex(address),"bytes":raw.hex(),"float32":value},"note":"Ranges include complete function bodies, with exclusive end boundaries immediately after final instruction bytes; padding bytes are excluded."}
Path("outputs/timeline-trim-handles-dataflow-verification.json").write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
print(json.dumps({"sha256":digest,"spans":spans,"total_bytes":result["total_verified_bytes"],"default_float":result["default_float"]},indent=2))
