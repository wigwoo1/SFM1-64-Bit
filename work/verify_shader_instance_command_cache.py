"""Build and verify the isolated owned-x64 per-instance command cache."""
from datetime import datetime, timezone
import hashlib
import json
import pathlib
import re
import struct
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT = ROOT / "outputs" / "shader-instance-cache-component-20261028"
BASE = ROOT / "outputs" / "shader-instance-cache-lifetime-20261028" / "verification-8t33kl8h" / "results.json"
EXPECTED_BASE_SHA = "3013ab9f8cf38b5fef46f92d381bb8118a817d32a8d4bb03fd3ffd6ec89aae47"
EXE_NAME = "probe_shader_instance_command_cache_x64.exe"
DUMPBIN = pathlib.Path(r"C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe")
U32 = struct.Struct("<I")
QUERY = struct.Struct("<12I")
HEADER = struct.Struct("<6I")
STATE_HEAD = struct.Struct("<8I")


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()


def checked_hashes(mapping, label):
    for name, expected in mapping.items():
        path = pathlib.Path(name)
        need(path.is_file() and sha(path) == expected,
             f"{label} hash mismatch or missing: {path}")


def make_words(query, phase, count, flags):
    seed = query[10]
    words = [((seed ^ (phase << 24) ^ (i * 0x9E3779B9)) & 0xFFFFFFFF)
             for i in range(count)]
    if flags & 0x40000:
        words.append(3)
    if flags & 2:
        words.append(4)
    words.append(0)
    return words


def extract_fresh_entry_corpus_checked(receipt, destination):
    # Verify exact current source evidence before parsing its query/trace format.
    runs = [r for r in receipt["runs"] if r["tag"] == "lifetime"]
    need(len(runs) == 2, "expected two accepted lifetime traces")
    trace = pathlib.Path(runs[0]["path"])
    need(sha(trace) == runs[0]["sha256"], "accepted lifetime trace hash")
    need(sha(trace) == "07bde07b4514c551b000576fd551da02c473750b73094a0bec6c4e7c7b75b7f2",
         "accepted lifetime trace identity")
    query_path = trace.parent / "queries.bin"
    need(query_path.is_file(), "lifetime queries missing")
    queries = query_path.read_bytes()
    trace_bytes = trace.read_bytes()
    need(len(queries) % QUERY.size == 0 and len(queries) // QUERY.size == 2400,
         "lifetime query count/shape")

    corpus = bytearray(struct.pack("<2I", 0x43414348, 0))
    cases = 0
    trace_at = 0
    for query_at in range(0, len(queries), QUERY.size):
        q = QUERY.unpack_from(queries, query_at)
        serial, kind, _capacity, n0, n1, flags0, flags1 = q[:7]
        header = HEADER.unpack_from(trace_bytes, trace_at)
        need(header[0] == serial, "trace serial mismatch")
        states_words, event_count = header[4], header[5]
        trace_at += HEADER.size
        states_at = trace_at
        snapshots = []
        consumed = 0
        while consumed < states_words:
            head = STATE_HEAD.unpack_from(trace_bytes, states_at + consumed * 4)
            data_count = head[7]
            size = 8 + data_count
            need(consumed + size <= states_words, "truncated snapshot state")
            words = struct.unpack_from("<" + "I" * data_count,
                                       trace_bytes, states_at + (consumed + 8) * 4) if data_count else ()
            snapshots.append((head, words))
            consumed += size
        trace_at += states_words * 4 + event_count * 9 * 4

        if kind != 0:
            continue
        need(len(snapshots) == 4, "fresh-entry trace should have four snapshots")
        first = snapshots[0]
        second = snapshots[1]
        input0 = make_words(q, 0, n0, flags0)
        input1 = make_words(q, 1, n1, flags1)
        cap0 = first[0][3] // 4
        cap1 = second[0][3] // 4
        need(first[0][3] % 4 == 0 and second[0][3] % 4 == 0,
             "reference byte capacities not word aligned")
        need(cap0 == len(input0) and first[1][:len(input0)] == tuple(input0),
             "reference first-list payload differs from decoded call")
        need(cap1 >= len(input1) and second[1][:len(input1)] == tuple(input1),
             "reference second-list payload differs from decoded call")
        need(len(first[1]) == cap0 and len(second[1]) == cap1,
             "reference full data span differs from capacity")
        corpus += U32.pack(serial)
        corpus += U32.pack(len(input0)) + U32.pack(cap0)
        corpus += struct.pack("<" + "I" * len(input0), *input0)
        corpus += struct.pack("<" + "I" * cap0, *first[1])
        corpus += U32.pack(len(input1)) + U32.pack(cap1)
        corpus += struct.pack("<" + "I" * len(input1), *input1)
        corpus += struct.pack("<" + "I" * cap1, *second[1])
        cases += 1
    need(trace_at == len(trace_bytes), "lifetime trace trailing bytes")
    need(cases == 400, f"expected 400 fresh-entry rows, got {cases}")
    corpus[4:8] = U32.pack(cases)
    destination.write_bytes(corpus)
    return cases, sha(destination)


def run(exe, args, cwd, label, exit_code=0):
    result = subprocess.run([str(exe), *map(str, args)], cwd=cwd,
                            capture_output=True, text=True, timeout=180)
    (cwd / f"{label}.log").write_text(result.stdout + result.stderr)
    need(result.returncode == exit_code,
         f"{label} exit={result.returncode}: {result.stdout}{result.stderr}")
    return result.stdout.strip()


def verify_pe(exe, build_log):
    raw = exe.read_bytes()
    pe = struct.unpack_from("<I", raw, 60)[0]
    machine = struct.unpack_from("<H", raw, pe + 4)[0]
    magic = struct.unpack_from("<H", raw, pe + 24)[0]
    need((machine, magic) == (0x8664, 0x20B), "probe is not AMD64/PE32+")
    need(b"asInvoker" in raw, "probe lacks non-elevated manifest")
    log = build_log.read_text(errors="replace")
    need(not re.search(r"\b(?:warning|error)\s+[A-Z]+\d+\s*:", log, re.I),
         "compiler warning/error in build log")
    imports = subprocess.run([str(DUMPBIN), "/imports", str(exe)],
                             capture_output=True, text=True, timeout=30)
    need(imports.returncode == 0, "dumpbin import inspection failed")
    dlls = sorted(set(re.findall(r"^\s+([^\s]+\.dll)\s*$", imports.stdout, re.I | re.M)))
    return machine, magic, dlls, imports.stdout


def run_regressions(receipt, run_dir):
    base_receipt = pathlib.Path(receipt["parent"])
    base = json.loads(base_receipt.read_text())
    prep_receipt = pathlib.Path(base["parent"])
    prep = json.loads(prep_receipt.read_text())
    blend_receipt = pathlib.Path(prep["parent"])
    blend = json.loads(blend_receipt.read_text())
    shader = next(pathlib.Path(p) for p in receipt["original_inputs"]
                  if pathlib.Path(p).name.lower() == "stdshader_dx9.dll")

    jobs = [
        ("commands_reference", receipt["executables"]["commands_reference"],
         shader, prep_receipt.parent, base_receipt.parent, base["analytical_sha256"]),
        ("commands_owned", receipt["executables"]["commands_owned"],
         None, prep_receipt.parent, base_receipt.parent, base["analytical_sha256"]),
        ("preparation_reference", receipt["executables"]["preparation_reference"],
         shader, prep_receipt.parent, prep_receipt.parent, prep["analytical_sha256"]),
        ("preparation_owned", receipt["executables"]["preparation_owned"],
         None, prep_receipt.parent, prep_receipt.parent, prep["analytical_sha256"]),
        ("blend", receipt["executables"]["blend"],
         None, blend_receipt.parent, blend_receipt.parent, blend["analytical_sha256"]),
    ]
    results = []
    for name, executable, original, mapping_dir, query_dir, expected in jobs:
        exe = pathlib.Path(executable["path"])
        need(sha(exe) == executable["sha256"], f"accepted regression executable changed: {name}")
        trace = run_dir / f"{name}.bin"
        args = ([original] if original else []) + [mapping_dir / "mapping.bin",
                query_dir / "queries.bin", trace]
        output = run(exe, args, run_dir, name)
        need(sha(trace) == expected, f"accepted regression trace mismatch: {name}")
        results.append({"name": name, "classification": "reused accepted executable",
                        "trace_sha256": sha(trace), "output": output})
    return results


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    run_dir = pathlib.Path(tempfile.mkdtemp(prefix="verification-", dir=OUT))
    print(f"Verification directory: {run_dir}", flush=True)
    need(sha(BASE) == EXPECTED_BASE_SHA, "accepted lifetime receipt hash")
    receipt = json.loads(BASE.read_text())
    protected = {**receipt["protected"], **receipt["source"], str(BASE): EXPECTED_BASE_SHA}
    protected.update({item["path"]: item["sha256"] for item in receipt["executables"].values()})
    checked_hashes(protected, "protected source/executable")
    checked_hashes(receipt["original_inputs"], "original reference input")

    corpus = run_dir / "fresh-entry-reference-cases.bin"
    ref_cases, corpus_sha = extract_fresh_entry_corpus_checked(receipt, corpus)
    build = subprocess.run(["cmd.exe", "/d", "/c",
                            str(ROOT / "work" / "build_shader_instance_command_cache_x64.cmd")],
                           cwd=ROOT, capture_output=True, text=True, timeout=180)
    (run_dir / "build-driver.log").write_text(build.stdout + build.stderr)
    need(build.returncode == 0, f"fresh x64 build failed: {build.stdout}{build.stderr}")
    exe = OUT / EXE_NAME
    build_log = OUT / "probe_shader_instance_command_cache_x64-build.log"
    machine, magic, dlls, import_text = verify_pe(exe, build_log)
    (run_dir / "probe-imports.txt").write_text(import_text)
    output = run(exe, [corpus], run_dir, "owned-cache-tests")
    injection_match = re.search(r"allocation_failure_injections=(\d+)", output)
    need(injection_match is not None, "missing allocation failure injection count")
    injection_count = int(injection_match.group(1))
    need(injection_count == 255,
         "expected one fresh failure per length and one growth failure per capacity")
    regressions = run_regressions(receipt, run_dir)

    # Existing application and earlier source evidence must remain unchanged.
    current_sources = {
        str(ROOT / "work" / "shader_instance_command_cache_candidate.h"): sha(ROOT / "work" / "shader_instance_command_cache_candidate.h"),
        str(ROOT / "work" / "probe_shader_instance_command_cache.cpp"): sha(ROOT / "work" / "probe_shader_instance_command_cache.cpp"),
        str(ROOT / "work" / "build_shader_instance_command_cache_x64.cmd"): sha(ROOT / "work" / "build_shader_instance_command_cache_x64.cmd"),
        str(ROOT / "work" / "verify_shader_instance_command_cache.py"): sha(ROOT / "work" / "verify_shader_instance_command_cache.py"),
    }
    checked_hashes(protected, "protected source/executable after tests")
    checked_hashes(receipt["original_inputs"], "original reference input after tests")

    result = {
        "status": "VERIFIED isolated owned x64 per-instance command-cache storage; not integrated into editor",
        "utc": datetime.now(timezone.utc).isoformat(),
        "reference_receipt": str(BASE),
        "reference_receipt_sha256": EXPECTED_BASE_SHA,
        "protected_identities_checked": len(protected),
        "original_inputs_checked": len(receipt["original_inputs"]),
        "new_source": current_sources,
        "fresh_x64_build": {"executable": str(exe), "sha256": sha(exe),
                            "machine": hex(machine), "optional_header": hex(magic),
                            "imports": dlls, "compiler_warnings": 0,
                            "build_log": str(build_log)},
        "reference_corpus": {"fresh_missing_entry_cases": ref_cases,
                             "file": str(corpus), "sha256": corpus_sha,
                             "comparison": "full storage words, used count, and logical high-water capacity for both stores"},
        "component_tests": output,
        "allocation_failure_injections": injection_count,
        "regressions": regressions,
        "editor_rebuilt": False,
        "limitations": [
            "The component is standalone and is not connected to the editor, renderer, snapshots, or per-pass ownership.",
            "Only successful storage behavior was compared to original traces. Allocation failure intentionally returns false safely; original failure paths faulted.",
            "The test allocator is single-threaded and requires nonthrowing equal-copy, deallocation, and destruction semantics.",
            "The reusable regression executables are unchanged accepted binaries; only the cache component probe was freshly compiled."
        ],
    }
    result_path = run_dir / "results.json"
    result_path.write_text(json.dumps(result, indent=2) + "\n")
    (OUT / "latest-verification.json").write_text(json.dumps(
        {"receipt": str(result_path), "receipt_sha256": sha(result_path)}, indent=2) + "\n")
    print(json.dumps({"status": result["status"], "receipt": str(result_path),
                      "executable_sha256": result["fresh_x64_build"]["sha256"],
                      "reference_cases": ref_cases,
                      "allocation_failure_injections": injection_count,
                      "regressions": [r["name"] for r in regressions]}, indent=2), flush=True)


if __name__ == "__main__":
    main()
