"""Validate compiled candidate resource layout; not a GPU execution test."""
import hashlib,json
from pathlib import Path
root=Path(__file__).resolve().parent.parent
assembly_path=root/'outputs/material-selfillum-shader-abi.spvasm'
assembly=assembly_path.read_text()
offsets=[96,100,104,108,112]
for index,offset in enumerate(offsets):
    assert f'OpMemberDecorate %type_ConstantBuffer_MaterialPush {index} Offset {offset}' in assembly
bindings=dict(baseTexture=0,baseSampler=1,selfIllumMask=2)
for name,binding in bindings.items():
    assert f'OpDecorate %{name} Binding {binding}' in assembly
    assert f'OpDecorate %{name} DescriptorSet 0' in assembly
binary=root/'build/session-core/sfm64-material-selfillum-candidate.spv'
data=binary.read_bytes();assert data[:4]==bytes.fromhex('03022307')
report=dict(push_offsets=offsets,push_member_end=124,bindings=bindings,
 binary_sha256=hashlib.sha256(data).hexdigest(),
 source_sha256=hashlib.sha256((root/'work/sfm64-material-selfillum-candidate.hlsl').read_bytes()).hexdigest(),
 scope='DXC generated SPIR-V layout only; app descriptor integration, gamma and actual GPU/native parity unfinished.')
(root/'outputs/material-selfillum-shader-abi-validation.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report))
