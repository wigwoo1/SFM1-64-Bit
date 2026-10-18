from pathlib import Path
import json
m=json.loads(Path('outputs/sfm64-engine-projection-chain-native-preparation.json').read_text());offset=0;locations={};chunks=[]
for r in m['routines']:
 offset=(offset+15)&~15;locations[int(r['original_start'],16)]=offset;chunks.append((r,offset));offset+=r['size']
checker=(offset+15)&~15;locations[0x102de362]=checker;offset=checker+16
constants={}
for target,bits in m['constants'].items():
 offset=(offset+15)&~15;constants[int(target,16)]=offset;offset+=16
header=['#pragma once','#include "engine_projection_chain_native_bytes.h"','inline void initializeProjectionChainNative(unsigned char* code){']
for r,where in chunks:header.append(f'std::memcpy(code+{where},{r["name"]}ChainBytes,{r["size"]});')
for target,bits in m['constants'].items():
 value=int(bits,16);where=constants[int(target,16)];header.append(f'{{uint32_t values[4]={{{value}u,{value}u,{value}u,{value}u}};std::memcpy(code+{where},values,16);}}')
# Original compare/branch/return sequence; mismatch goes to an owned trap.
header.append(f'{{unsigned char check[]={{0x3b,0x0d,0,0,0,0,0x75,0x02,0xf3,0xc3,0xcc,0xc3}};std::memcpy(code+{checker},check,sizeof(check));}}')
header.append(f'{{uint32_t address=reinterpret_cast<uint32_t>(code+{constants[0x10415d00]});std::memcpy(code+{checker+2},&address,4);}}')
for r,where in chunks:
 for fix in r['relocations']:
  at=where+fix['offset'];target=int(fix['target'],16)
  if fix['kind']=='absolute':header.append(f'{{uint32_t address=reinterpret_cast<uint32_t>(code+{constants[target]});std::memcpy(code+{at},&address,4);}}')
  else:header.append(f'{{int32_t displacement={locations[target]-(at+4)};std::memcpy(code+{at},&displacement,4);}}')
header+=['}',f'inline constexpr unsigned projectionChainAllocationSize={offset};']
Path('work/engine_projection_chain_native_init.h').write_text('\n'.join(header)+'\n')
print('allocation bytes',offset)
