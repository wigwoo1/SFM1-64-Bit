"""Explain the retained transparency failure with independent triangle coverage.

Reads the generated fixture, prepared CPU camera/indices and old GPU capture.
Does not render, modify the original fixture or use any original SFM files.
"""
from collections import Counter
from pathlib import Path
import hashlib
import json
import math
import re
import struct
import subprocess
import sys
from translucent_fixture_geometry_candidate import (
    ordered_faces, validate_translucent_quad, repair_translucent_fixture_quad)

ROOT = Path(__file__).resolve().parent.parent
FAILED = ROOT / 'outputs/viewport-output-blending-4k9rut5t/verification-_mp56xn2'
CAPTURE_SHA = '0ff66471fed4baa6522e056dd9fb945947af831a6284d84ee4a85001235748e6'


def need(condition, message):
    if not condition: raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def geometry_guard(data):
    refused = False
    try: validate_translucent_quad(data, 1)
    except ValueError: refused = True
    need(refused, 'Malformed inherited quad was not rejected')
    repaired = bytearray(data)
    repair_translucent_fixture_quad(repaired)
    good = validate_translucent_quad(repaired, 1)
    # A disconnected loop and a collapsed edge must also fail before rendering.
    edges = struct.unpack_from('<I', repaired, 8+12*16)[0]
    for endpoint in [(0,2), (3,3)]:
        corrupt = bytearray(repaired)
        struct.pack_into('<HH', corrupt, edges+5*4, *endpoint)
        refused = False
        try: validate_translucent_quad(corrupt, 1)
        except ValueError: refused = True
        need(refused, 'Malformed edge-loop control accepted')
    return good


def main():
    need(len(sys.argv) == 2, 'Supply the isolated topology output directory')
    parent = Path(sys.argv[1]).resolve()
    need(parent.is_relative_to(ROOT/'outputs'), 'Workspace outputs only')
    executable = parent/'cpu-probe/translucent_fixture_geometry_x64.exe'
    fixture = FAILED/'fixtures/layers-1-alpha-64'
    bsp = fixture/'maps/owned.bsp'
    capture = FAILED/'editor-layers-1-alpha-64/vulkan-retained-frame-0.rgba'
    need(sha(capture) == CAPTURE_SHA, 'Retained failing GPU capture changed')
    data = bsp.read_bytes()
    face = ordered_faces(data)[0]
    need(face == [(0.,0.,0.),(2.,0.,0.),(2.,2.,0.),(3.,2.,0.)], 'Expected inherited topology')
    good = geometry_guard(data)
    process = subprocess.run([str(executable),str(fixture),str(fixture/'113-nearest-1.profile')],
        capture_output=True,text=True,timeout=30,creationflags=subprocess.CREATE_NO_WINDOW)
    (parent/'old-geometry.log').write_text(process.stdout+process.stderr)
    need(process.returncode == 0, 'Prepared CPU geometry probe failed')
    camera = None; vertices = {}; triangles = []
    for line in process.stdout.splitlines():
        columns = line.split()
        if columns[0] == 'camera': camera = list(map(float,columns[1:]))
        elif columns[0] == 'vertex': vertices[int(columns[1])] = tuple(map(float,columns[2:]))
        elif columns[0] == 'triangle': triangles.append(list(map(int,columns[1:])))
    need(camera and len(camera) == 16 and list(vertices.values()) == face and
         triangles == [[0,1,2],[0,2,3]], 'CPU upload differs from independent BSP edge walk')
    projected = {}
    for index, vertex in vertices.items():
        clip = [sum(camera[row*4+axis]*([*vertex,1][axis]) for axis in range(4)) for row in range(4)]
        need(clip[3] > 0 and 0 < clip[2] < clip[3], 'Fixture crosses depth clipping plane')
        projected[index] = ((clip[0]/clip[3]+1)*256,(clip[1]/clip[3]+1)*256)
    def interior(triangle, x, y):
        p = [projected[index] for index in triangle]
        sides = [(p[(n+1)%3][0]-p[n][0])*(y-p[n][1]) -
                 (p[(n+1)%3][1]-p[n][1])*(x-p[n][0]) for n in range(3)]
        return all(v > 0 for v in sides) or all(v < 0 for v in sides)
    wire = capture.read_bytes()
    need(len(wire) == 512*512*4, 'Retained capture extent')
    counts = Counter(); mismatches = 0
    colors = [(63,63,80,255),(146,146,151,255),(182,182,185,255)]
    for y in range(512):
        for x in range(512):
            layers = sum(interior(triangle,x+.5,y+.5) for triangle in triangles)
            counts[layers] += 1
            actual = tuple(wire[(y*512+x)*4:(y*512+x+1)*4])
            mismatches += actual != colors[layers]
    need(dict(counts) == {0:259791,1:1663,2:690} and mismatches == 0,
         'Independent triangle coverage does not explain the retained failure')
    raw = executable.read_bytes(); offset = struct.unpack_from('<I',raw,60)[0]
    need(raw[offset:offset+4] == b'PE\0\0' and struct.unpack_from('<H',raw,offset+4)[0] == 0x8664 and
         struct.unpack_from('<H',raw,offset+24)[0] == 0x20b, 'CPU probe AMD64 PE32+')
    warnings = re.findall(r'warning (C\d+): ([^\r\n]+)',(executable.parent/'build.log').read_text(errors='replace'))
    result = dict(status='VERIFIED malformed authored quad explains every old capture pixel',
        prepared_cpu_probe=dict(path=str(executable),sha256=sha(executable),compiler_warnings=warnings),
        source_sha256={name:sha(ROOT/'work'/name) for name in [Path(__file__).name,
            'translucent_fixture_geometry_candidate.py','probe_translucent_fixture_geometry_x64.cpp',
            'build_translucent_fixture_geometry_x64.cmd']},
        original_fixture_sha256=sha(bsp),retained_capture_sha256=sha(capture),
        ordered_vertices=face,fan_triangles=triangles,projected_vertices=projected,
        predicted_pixel_counts=dict(counts),mismatched_pixels=mismatches,
        corrected_quad=good,negative_controls=3,
        limits=['Prepared camera and upload data use the existing CPU scene evaluator; the BSP edge walk and pixel coverage calculation are independent Python.',
                'Exact coverage observed for this fixed fixture and captured frame, not a general Vulkan rasterizer conformance oracle.',
                'No original SFM data, process or installation used. Production code unchanged.'])
    (parent/'topology-results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({key:result[key] for key in ['status','predicted_pixel_counts','mismatched_pixels','negative_controls']},indent=2))


if __name__ == '__main__': main()
